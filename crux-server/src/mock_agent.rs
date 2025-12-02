use crate::api::pastebin::models::AgentMessage;
use tokio::sync::mpsc;
use std::time::Duration;
use tokio::time::sleep;

pub async fn handle_mock_command(command: &str, tx: mpsc::Sender<AgentMessage>) {
    // Simulate network/processing delay
    sleep(Duration::from_secs(1)).await;

    let response = match command.trim() {
        "ls" => {
            let files = vec![
                "C:\\Users\\Admin\\Documents\\passwords.txt",
                "C:\\Windows\\System32\\config\\SAM",
                "C:\\Program Files\\Crux\\agent.exe",
                "D:\\Backups\\database.sql",
                "C:\\Users\\Public\\payload.bin",
            ];
            Some(AgentMessage {
                msg_type: "file_list".to_string(),
                payload: serde_json::to_string(&files).unwrap(),
                timestamp: Some(chrono::Utc::now().timestamp()),
            })
        }
        "ps" => {
            let procs = vec![
                "System (4)",
                "smss.exe (348)",
                "csrss.exe (520)",
                "wininit.exe (612)",
                "services.exe (792)",
                "lsass.exe (808)",
                "svchost.exe (904)",
                "explorer.exe (4500)",
                "chrome.exe (2100)",
                "crux-agent.exe (9999)",
            ];
            Some(AgentMessage {
                msg_type: "process_list".to_string(),
                payload: serde_json::to_string(&procs).unwrap(),
                timestamp: Some(chrono::Utc::now().timestamp()),
            })
        }
        "whoami" => Some(AgentMessage {
            msg_type: "output".to_string(),
            payload: "nt authority\\system".to_string(),
            timestamp: Some(chrono::Utc::now().timestamp()),
        }),
        "ipconfig" => Some(AgentMessage {
            msg_type: "output".to_string(),
            payload: "Ethernet adapter Ethernet:\n\n   Connection-specific DNS Suffix  . : localdomain\n   IPv4 Address. . . . . . . . . . . : 192.168.1.105\n   Subnet Mask . . . . . . . . . . . : 255.255.255.0\n   Default Gateway . . . . . . . . . : 192.168.1.1".to_string(),
            timestamp: Some(chrono::Utc::now().timestamp()),
        }),
        "net user" => Some(AgentMessage {
            msg_type: "output".to_string(),
            payload: "User accounts for \\\\WORKSTATION-01\n\n-------------------------------------------------------------------------------\nAdministrator            DefaultAccount           Guest                    \nwdagutilityaccount       Kali                     \nThe command completed successfully.".to_string(),
            timestamp: Some(chrono::Utc::now().timestamp()),
        }),
        _ => None, // Ignore unknown commands or let them be
    };

    if let Some(msg) = response {
        let _ = tx.send(msg).await;
    }
}
