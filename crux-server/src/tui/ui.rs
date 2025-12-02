use crate::crux::state::AppState;
use crate::crux::state::ViewMode;
use crate::MenuContext;
use ratatui::prelude::*;
use ratatui::widgets::*;

pub fn render(frame: &mut Frame, app_state: &AppState) {
    let [frame_area] = Layout::vertical([Constraint::Fill(1)])
        .margin(1)
        .areas(frame.area());

    Block::default()
        .borders(Borders::ALL)
        .border_type(BorderType::Rounded)
        .title("Crux:25 Server")
        .fg(Color::Green)
        .render(frame.area(), frame.buffer_mut());

    match app_state.view_mode {
        ViewMode::Dashboard => render_dashboard(frame, app_state, frame_area),
        ViewMode::FileBrowser => render_file_browser(frame, app_state, frame_area),
        ViewMode::ProcessList => render_process_list(frame, app_state, frame_area),
        ViewMode::Logs => render_logs(frame, app_state, frame_area),
        ViewMode::PayloadList => render_payload_list(frame, app_state, frame_area),
    }

    if app_state.menu_state.active {
        render_menu(frame, app_state, frame_area);
    }
}

fn render_dashboard(frame: &mut Frame, app_state: &AppState, area: Rect) {
    let vertical_chunks = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([Constraint::Percentage(50), Constraint::Percentage(50)])
        .margin(1)
        .split(area);

    let left = vertical_chunks[0];
    let right = vertical_chunks[1];

    Block::default()
        .borders(Borders::ALL)
        .border_type(BorderType::Rounded)
        .title("Live Hosts")
        .render(left, frame.buffer_mut());

    let left_inner = left.inner(Margin::new(1, 1));

    let list = List::new(app_state.pastes.iter().map(|paste| {
        // Calculate time since check-in (mock logic since we don't have real time sync yet)
        // In real scenario: let now = chrono::Utc::now().timestamp();
        // let diff = now - paste.paste_date;
        // For now, just show the date
        
        let status = if paste.paste_date > 0 { "🟢" } else { "🔴" };
        
        ListItem::new(format!(
            "{} {}: {}",
            status,
            paste.paste_title,
            paste.paste_url.as_str()
        ))
    }))
    .highlight_style(Style::default().add_modifier(Modifier::BOLD).fg(Color::Cyan))
    .fg(Color::Yellow);

    frame.render_stateful_widget(list, left_inner, &mut app_state.list_state.clone());

    let right_chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Percentage(30), // Logs (Top)
            Constraint::Percentage(50), // Host Data (Middle)
            Constraint::Percentage(20), // Command (Bottom)
        ])
        .split(right);

    let top = right_chunks[0];
    let middle = right_chunks[1];
    let bottom = right_chunks[2];

    // Logs (Top)
    let logs: Vec<ListItem> = app_state
        .logs
        .iter()
        .rev()
        .take(20) // Show more logs
        .map(|log| {
             let style = if log.level == "ERROR" {
                Style::default().fg(Color::Red)
            } else if log.level == "WARN" {
                Style::default().fg(Color::Yellow)
            } else if log.level == "DEBUG" {
                Style::default().fg(Color::DarkGray)
            } else {
                Style::default().fg(Color::Gray)
            };
            
            let msg = if log.count > 1 {
                format!("{} (x{}) - {}", log.level, log.count, log.message)
            } else {
                format!("{} - {}", log.level, log.message)
            };

            ListItem::new(msg).style(style)
        })
        .collect();

    let logs_list = List::new(logs)
        .block(Block::default().borders(Borders::ALL).title("Server Logs"))
        .fg(Color::Gray);
    
    frame.render_widget(logs_list, top);

    // Host Data (Middle)
    let mut content_text = String::new();
    let mut msg_type = String::from("Unknown");

    if let Some(selected_index) = app_state.list_state.selected() {
        if let Some(paste) = app_state.pastes.get(selected_index) {
            if let Some(agent_msg) = app_state.paste_content.get(&paste.paste_key) {
                content_text = agent_msg.payload.clone();
                msg_type = agent_msg.msg_type.clone();
            }
        }
    }

    let paragraph = Paragraph::new(content_text)
        .wrap(Wrap { trim: true });
    
    let title = format!("Host Data ({})", msg_type);

    frame.render_widget(paragraph.block(Block::default().borders(Borders::ALL).title(title.as_str())), middle);

    let input_style = if app_state.input_mode {
        Style::default().fg(Color::Yellow)
    } else {
        Style::default()
    };

    let input = Paragraph::new(app_state.input_buffer.as_str())
        .style(input_style)
        .block(Block::default().borders(Borders::ALL).title("Host Command"));

    frame.render_widget(input, bottom);
}

fn render_file_browser(frame: &mut Frame, app_state: &AppState, area: Rect) {
    let chunks = Layout::default()
        .margin(1)
        .constraints([Constraint::Percentage(100)])
        .split(area);

    let items: Vec<ListItem> = app_state.file_list.iter()
        .map(|f| ListItem::new(f.as_str()))
        .collect();

    let list = List::new(items)
        .block(Block::default().borders(Borders::ALL).title("File Browser"))
        .highlight_style(Style::default().add_modifier(Modifier::BOLD).fg(Color::Cyan));
    
    frame.render_widget(list, chunks[0]);
}

fn render_process_list(frame: &mut Frame, app_state: &AppState, area: Rect) {
    let chunks = Layout::default()
        .margin(1)
        .constraints([Constraint::Percentage(100)])
        .split(area);

    let items: Vec<ListItem> = app_state.process_list.iter()
        .map(|p| ListItem::new(p.as_str()))
        .collect();

    let list = List::new(items)
        .block(Block::default().borders(Borders::ALL).title("Process List"))
        .highlight_style(Style::default().add_modifier(Modifier::BOLD).fg(Color::Cyan));
    
    frame.render_widget(list, chunks[0]);
}

fn render_logs(frame: &mut Frame, app_state: &AppState, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Horizontal)
        .margin(1)
        .constraints([
            Constraint::Percentage(50), // Activity Logs
            Constraint::Percentage(50), // System/Debug Logs
        ])
        .split(area);

    // Activity Logs (Left) - Filter for C2 events
    let activity_logs: Vec<ListItem> = app_state.logs.iter()
        .rev()
        .filter(|log| {
            let msg = &log.message;
            msg.contains("Command sent") || 
            msg.contains("Paste dropped") || 
            msg.contains("Agent registered") || 
            msg.contains("Task") ||
            msg.contains("Output:")
        })
        .map(|log| {
            let style = Style::default().fg(Color::Green);
            let msg = format!("{} - {}", log.timestamp, log.message);
            ListItem::new(msg).style(style)
        })
        .collect();

    let activity_list = List::new(activity_logs)
        .block(Block::default().borders(Borders::ALL).title("C2 Activity"))
        .highlight_style(Style::default().add_modifier(Modifier::BOLD).fg(Color::Cyan));
    
    frame.render_widget(activity_list, chunks[0]);

    // System Logs (Right) - Show everything (or just debug/info)
    let system_logs: Vec<ListItem> = app_state.logs.iter()
        .rev()
        .map(|log| {
            let style = if log.level == "ERROR" {
                Style::default().fg(Color::Red)
            } else if log.level == "WARN" {
                Style::default().fg(Color::Yellow)
            } else if log.level == "DEBUG" {
                Style::default().fg(Color::DarkGray)
            } else {
                Style::default().fg(Color::Gray)
            };
            
            let msg = if log.count > 1 {
                format!("{} (x{}) - {}", log.level, log.count, log.message)
            } else {
                format!("{} - {}", log.level, log.message)
            };

            ListItem::new(msg).style(style)
        })
        .collect();

    let system_list = List::new(system_logs)
        .block(Block::default().borders(Borders::ALL).title("System / Debug Logs"))
        .highlight_style(Style::default().add_modifier(Modifier::BOLD).fg(Color::Cyan));
    
    frame.render_widget(system_list, chunks[1]);
}

fn render_payload_list(frame: &mut Frame, app_state: &AppState, area: Rect) {
    let items: Vec<ListItem> = app_state.payload_list.iter()
        .map(|payload| {
            ListItem::new(payload.clone()).style(Style::default().fg(Color::Yellow))
        })
        .collect();

    let list = List::new(items)
        .block(Block::default().borders(Borders::ALL).title("Available Payloads"))
        .highlight_style(Style::default().add_modifier(Modifier::BOLD).fg(Color::Cyan));
    
    // We reuse list_state for selection if we want to allow selecting a payload
    // For now just list them
    frame.render_widget(list, area);
}

fn render_menu(frame: &mut Frame, app_state: &AppState, area: Rect) {
    let area = centered_rect(60, 20, area);
    
    match app_state.menu_state.context {
        MenuContext::QuickCommand => {
            let input = Paragraph::new(app_state.menu_input_buffer.as_str())
                .style(Style::default().fg(Color::Yellow))
                .block(Block::default().borders(Borders::ALL).title("Quick Command").bg(Color::DarkGray));
            
            frame.render_widget(Clear, area);
            frame.render_widget(input, area);
        }
        _ => {
            let items: Vec<ListItem> = app_state.menu_state.items.iter().enumerate()
                .map(|(i, item)| {
                    let style = if i == app_state.menu_state.selected_index {
                        Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD)
                    } else {
                        Style::default()
                    };
                    ListItem::new(item.as_str()).style(style)
                })
                .collect();

            let list = List::new(items)
                .block(Block::default().borders(Borders::ALL).title(
                    match app_state.menu_state.context {
                        MenuContext::Main => "Menu",
                        MenuContext::HostCommands => "Host Commands",
                        _ => "Menu",
                    }
                ).bg(Color::DarkGray));
            
            frame.render_widget(Clear, area); // Clear background
            frame.render_widget(list, area);
        }
    }
}

fn centered_rect(percent_x: u16, percent_y: u16, r: Rect) -> Rect {
    let popup_layout = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Percentage((100 - percent_y) / 2),
            Constraint::Percentage(percent_y),
            Constraint::Percentage((100 - percent_y) / 2),
        ])
        .split(r);

    Layout::default()
        .direction(Direction::Horizontal)
        .constraints([
            Constraint::Percentage((100 - percent_x) / 2),
            Constraint::Percentage(percent_x),
            Constraint::Percentage((100 - percent_x) / 2),
        ])
        .split(popup_layout[1])[1]
}
