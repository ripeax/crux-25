use crate::api::pastebin::models::{
    DeleteRequest, ListRequest, PasteList, PasteRequest, PastebinDataRX, ShowRequest,
};
use anyhow::Error;
use quick_xml::de::from_str;
use reqwest::Client;

const API_POST_URL: &str = "https://pastebin.com/api/api_post.php";
const API_RAW_URL: &str = "https://pastebin.com/api/api_raw.php";

pub async fn get_paste(paste_id: &str) -> Result<PastebinDataRX, Error> {
    let client = Client::new();
    let req = ShowRequest::new(paste_id);

    let resp = client.post(API_RAW_URL).form(&req).send().await?;

    Ok(resp.json().await?)
}

pub async fn paste_list() -> Result<PasteList, Error> {
    let client = Client::new();
    let req = ListRequest::new();

    let resp = client.post(API_POST_URL).form(&req).send().await?;

    let wrapped = format!("<root>{}</root>", resp.text().await?);
    let parsed: PasteList = from_str(&wrapped).unwrap_or_default();

    Ok(parsed)
}

pub async fn post_paste(data: &str) -> Result<String, Error> {
    let client = Client::new();
    let req = PasteRequest::new(data);

    let resp = client.post(API_POST_URL).form(&req).send().await?;

    Ok(resp.text().await?)
}

pub async fn delete_paste(paste_id: &str) -> Result<String, Error> {
    let client = Client::new();
    let req = DeleteRequest::new(paste_id);

    let resp = client.post(API_POST_URL).form(&req).send().await?;

    Ok(resp.text().await?)
}
