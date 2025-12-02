use sqlx::{sqlite::{SqlitePoolOptions, SqliteConnectOptions}, Pool, Sqlite};
use std::str::FromStr;
use anyhow::Result;

pub async fn init_db(database_url: &str) -> Result<Pool<Sqlite>> {
    let options = SqliteConnectOptions::from_str(database_url)?
        .create_if_missing(true);

    let pool = SqlitePoolOptions::new()
        .max_connections(5)
        .connect_with(options)
        .await?;

    sqlx::query(
        "CREATE TABLE IF NOT EXISTS pastes (
            key TEXT PRIMARY KEY,
            content TEXT NOT NULL,
            created_at INTEGER NOT NULL
        );"
    )
    .execute(&pool)
    .await?;

    sqlx::query(
        "CREATE TABLE IF NOT EXISTS tasks (
            id TEXT PRIMARY KEY,
            agent_id TEXT NOT NULL,
            command TEXT NOT NULL,
            status TEXT NOT NULL,
            created_at INTEGER NOT NULL
        );"
    )
    .execute(&pool)
    .await?;

    sqlx::query(
        "CREATE TABLE IF NOT EXISTS results (
            id TEXT PRIMARY KEY,
            task_id TEXT NOT NULL,
            output TEXT NOT NULL,
            created_at INTEGER NOT NULL
        );"
    )
    .execute(&pool)
    .await?;

    Ok(pool)
}

pub async fn save_paste(pool: &Pool<Sqlite>, key: &str, content: &str) -> Result<()> {
    let now = chrono::Utc::now().timestamp();
    sqlx::query("INSERT OR REPLACE INTO pastes (key, content, created_at) VALUES (?, ?, ?)")
        .bind(key)
        .bind(content)
        .bind(now)
        .execute(pool)
        .await?;
    Ok(())
}

pub async fn get_paste(pool: &Pool<Sqlite>, key: &str) -> Result<Option<String>> {
    let result = sqlx::query_as::<_, (String,)>("SELECT content FROM pastes WHERE key = ?")
        .bind(key)
        .fetch_optional(pool)
        .await?;
    
    Ok(result.map(|r| r.0))
}
