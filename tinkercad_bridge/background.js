let bridgeToken = "dev-secret-token-123";

// Keep token synced in background
chrome.storage.sync.get(['bridgeToken'], function (result) {
    if (result.bridgeToken) {
        bridgeToken = result.bridgeToken;
    }
});

chrome.storage.onChanged.addListener(function (changes, namespace) {
    if (changes.bridgeToken) {
        bridgeToken = changes.bridgeToken.newValue;
    }
});

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
    if (request.action === "sendToIngest") {
        fetch("http://127.0.0.1:8080/api/ingest/tinkercad", {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": `Bearer ${bridgeToken}`
            },
            body: JSON.stringify({ raw_line: request.dataLine })
        })
            .then(response => {
                if (!response.ok) {
                    console.error("[Tinkercad Background] Ingest failed:", response.status);
                } else {
                    console.log("[Tinkercad Background] Successfully sent data line.");
                }
            })
            .catch(err => console.error("[Tinkercad Background] Fetch error:", err));

        // Return true to indicate asynchronous response if needed later
        return true;
    }
});
