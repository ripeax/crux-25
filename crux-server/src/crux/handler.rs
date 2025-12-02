use crate::api::pastebin::models::Paste;
use async_trait::async_trait;

#[async_trait]
pub trait EventHandler: Send + Sync {
    async fn on_paste(&self, paste: Paste);
}
