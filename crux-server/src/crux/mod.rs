pub mod handler;
pub mod state;
pub mod models;

use crate::api::pastebin::requests::{delete_paste, paste_list};
use crate::crux::handler::EventHandler;
use std::time::Duration;
use tokio::time::sleep;

pub async fn serve<H>(handler: H)
where
    H: EventHandler + Send + Sync + 'static,
{
    loop {
        let paste_list = paste_list().await.unwrap();

        for paste in paste_list.paste {
            delete_paste(&paste.paste_key).await.unwrap();
            handler.on_paste(paste).await;
        }

        sleep(Duration::from_secs(1)).await;
    }
}
