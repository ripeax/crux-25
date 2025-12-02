use std::sync::{Arc, Mutex};
use tracing::{Event, Subscriber};
use tracing_subscriber::{layer::Context, Layer, registry::LookupSpan};
use crate::crux::state::AppState;

pub struct TuiLogger {
    app_state: Arc<tokio::sync::Mutex<AppState>>,
}

impl TuiLogger {
    pub fn new(app_state: Arc<tokio::sync::Mutex<AppState>>) -> Self {
        Self { app_state }
    }
}

impl<S> Layer<S> for TuiLogger
where
    S: Subscriber + for<'a> LookupSpan<'a>,
{
    fn on_event(&self, event: &Event<'_>, _ctx: Context<'_, S>) {
        let mut visitor = MessageVisitor::new();
        event.record(&mut visitor);
        
        let message = visitor.message;
        let level = event.metadata().level().to_string();
        
        let state = self.app_state.clone();
        tokio::spawn(async move {
            let mut state = state.lock().await;
            state.log(&message, &level);
        });
    }
}

struct MessageVisitor {
    message: String,
}

impl MessageVisitor {
    fn new() -> Self {
        Self { message: String::new() }
    }
}

impl tracing::field::Visit for MessageVisitor {
    fn record_debug(&mut self, field: &tracing::field::Field, value: &dyn std::fmt::Debug) {
        if field.name() == "message" {
            self.message = format!("{:?}", value);
        }
    }
}
