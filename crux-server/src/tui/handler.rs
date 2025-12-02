use ratatui::crossterm::event;
use ratatui::crossterm::event::{Event, KeyCode};

pub enum InputType {
    None,
    Quit,
    Up,
    Down,
    Enter,
    Esc,
    Backspace,
    Char(char),
}

pub fn input() -> InputType {
    // Use non-blocking poll with a short timeout (e.g., 50ms)
    // If no event is available, return InputType::None so the UI can continue redrawing.
    if event::poll(std::time::Duration::from_millis(50)).unwrap_or(false) {
        if let Event::Key(key) = event::read().unwrap() {
            match key.code {
                KeyCode::Char('q') => InputType::Quit,
                KeyCode::Up => InputType::Up,
                KeyCode::Down => InputType::Down,
                KeyCode::Enter => InputType::Enter,
                KeyCode::Esc => InputType::Esc,
                KeyCode::Backspace => InputType::Backspace,
                KeyCode::Char(c) => InputType::Char(c),
                _ => InputType::None,
            }
        } else {
            InputType::None
        }
    } else {
        InputType::None
    }
}
