use std::sync::Arc;
use std::collections::HashMap;
use tokio::sync::mpsc;
use ratatui::widgets::ListState;
use crate::api::pastebin::models::{AgentMessage, Paste};
use crate::crux::models::{Agent, Task};
use crate::mock_agent;
use sqlx::{Pool, Sqlite};

#[derive(Clone, PartialEq, Debug)]
pub enum ViewMode {
    Dashboard,
    FileBrowser,
    ProcessList,
    Logs,
    PayloadList,
}

#[derive(Clone, Debug)]
pub struct LogEntry {
    pub message: String,
    pub level: String,
    pub count: usize,
    pub timestamp: i64,
}


#[derive(Clone, PartialEq)]
pub enum MenuContext {
    Main,
    HostCommands,
    QuickCommand,
}

#[derive(Clone)]
pub struct MenuState {
    pub active: bool,
    pub context: MenuContext,
    pub items: Vec<String>,
    pub selected_index: usize,
}

#[derive(Clone)]
pub struct AppState {
    pub pastes: Vec<Arc<Paste>>,
    pub paste_content: HashMap<String, AgentMessage>,
    pub list_state: ListState,
    pub input_buffer: String,
    pub input_mode: bool,
    pub logs: Vec<LogEntry>,
    pub command_history: Vec<String>,
    pub history_index: usize,
    pub menu_state: MenuState,
    pub menu_input_buffer: String,
    pub view_mode: ViewMode,
    // Direct C2 Data
    pub agents: HashMap<String, Agent>,
    pub tasks: HashMap<String, Vec<Task>>, // AgentID -> Tasks
    // Mock Data for views
    pub file_list: Vec<String>,
    pub process_list: Vec<String>,
    pub payload_list: Vec<String>,
    pub db: Pool<Sqlite>,
}

impl AppState {
    pub fn log(&mut self, message: &str, level: &str) {
        let message = message.to_string();
        let level = level.to_string();
        
        let mut is_duplicate = false;
        if let Some(last) = self.logs.last_mut() {
            if last.message == message && last.level == level {
                last.count += 1;
                last.timestamp = chrono::Utc::now().timestamp();
                is_duplicate = true;
            }
        }

        if !is_duplicate {
            self.logs.push(LogEntry {
                message,
                level,
                count: 1,
                timestamp: chrono::Utc::now().timestamp(),
            });
            // Keep log size manageable
            if self.logs.len() > 1000 {
                self.logs.remove(0);
            }
        }
    }
}

pub async fn process_agent_message(state: &mut AppState, agent_msg: AgentMessage, mock_tx: mpsc::Sender<AgentMessage>) {
     // Handle Dynamic Updates
     match agent_msg.msg_type.as_str() {
         "file_list" => {
             if let Ok(files) = serde_json::from_str::<Vec<String>>(&agent_msg.payload) {
                 state.file_list = files;
             }
         },
         "process_list" => {
             if let Ok(procs) = serde_json::from_str::<Vec<String>>(&agent_msg.payload) {
                 state.process_list = procs;
             }
         },
         "output" => {
             state.log(&format!("Output: {}", agent_msg.payload), "INFO");
         },
         "raw" => {
             // It's a command from us (or another server)
             // Trigger Mock Agent to respond
             let command = agent_msg.payload.clone();
             let tx = mock_tx.clone();
             tokio::spawn(async move {
                 mock_agent::handle_mock_command(&command, tx).await;
             });
         }
         _ => {}
     }
}
