use crate::api::pastebin::{API_KEY, USER_KEY};
use serde::{Deserialize, Serialize};

#[derive(Deserialize)]
pub struct Paste {
    pub paste_key: String,
    pub paste_date: i64,
    pub paste_title: String,
    pub paste_size: u64,
    pub paste_expire_date: i64,
    pub paste_private: u8,
    pub paste_format_long: String,
    pub paste_format_short: String,
    pub paste_url: String,
    pub paste_hits: u64,
}

#[derive(Deserialize, Default)]
pub struct PasteList {
    pub paste: Vec<Paste>,
}

#[derive(Serialize)]
pub struct DeleteRequest {
    api_dev_key: String,
    api_user_key: String,
    api_paste_key: String,
    api_option: String,
}

impl DeleteRequest {
    pub fn new(paste_id: &str) -> Self {
        Self {
            api_dev_key: String::from(API_KEY),
            api_user_key: String::from(USER_KEY),
            api_paste_key: String::from(paste_id),
            api_option: String::from("delete"),
        }
    }
}

#[derive(Serialize)]
pub struct ListRequest {
    api_dev_key: String,
    api_user_key: String,
    api_option: String,
}

impl ListRequest {
    pub fn new() -> Self {
        Self {
            api_dev_key: String::from(API_KEY),
            api_user_key: String::from(USER_KEY),
            api_option: String::from("list"),
        }
    }
}

#[derive(Serialize)]
pub struct PasteRequest {
    api_dev_key: String,
    api_paste_code: String,
    api_paste_private: u8,
    api_user_key: String,
    api_option: String,
}

impl PasteRequest {
    pub fn new(data: &str) -> Self {
        Self {
            api_user_key: String::from(USER_KEY),
            api_paste_code: String::from(data),
            api_paste_private: 2,
            api_dev_key: String::from(API_KEY),
            api_option: String::from("paste"),
        }
    }
}

#[derive(Serialize)]
pub struct ShowRequest {
    api_dev_key: String,
    api_user_key: String,
    api_paste_key: String,
    api_option: String,
}

impl ShowRequest {
    pub fn new(paste_id: &str) -> Self {
        Self {
            api_dev_key: String::from(API_KEY),
            api_user_key: String::from(USER_KEY),
            api_paste_key: String::from(paste_id),
            api_option: String::from("show_paste"),
        }
    }
}

#[derive(Deserialize, Serialize, Debug, Clone)]
pub struct AgentMessage {
    pub msg_type: String,
    pub payload: String,
    pub timestamp: Option<i64>,
}

#[derive(Deserialize, Debug)]
pub struct PastebinDataRX {
    pub hello: String,
}
