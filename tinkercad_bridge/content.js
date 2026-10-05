console.log("[Tinkercad Bridge] Content script loaded inside context:", window.location.href);

let bridgeToken = "dev-secret-token-123";
let lastSentText = "";
let lastSentTime = 0;

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

async function sendToIngest(dataLine) {
    console.log("[Tinkercad Bridge] Capturing and Sending through Background Worker:", dataLine);
    try {
        chrome.runtime.sendMessage({ action: "sendToIngest", dataLine: dataLine });
    } catch (err) {
        console.error("[Tinkercad Bridge] Request error:", err);
    }
}

// Check text directly and send if it contains the sensor readings
function processText(text) {
    if (!text) return;
    if (text.includes("Temp:") && text.includes("Hum:") && text.includes("Soil:") && text.includes("Light:")) {
        const now = Date.now();
        // Send if text changed, or if it's the exact same reading but at least 2 seconds passed
        if (text !== lastSentText || (now - lastSentTime > 2000)) {
            lastSentText = text;
            lastSentTime = now;
            sendToIngest(text);
        }
    }
}

// 1. MutationObserver for responsive capture (catching appended elements or text changes)
const observer = new MutationObserver((mutations) => {
    for (const mutation of mutations) {
        if (mutation.type === "childList") {
            mutation.addedNodes.forEach(node => {
                let text = node.textContent || node.value || "";
                processText(text.trim());
            });
        } else if (mutation.type === "characterData") {
            processText((mutation.target.textContent || "").trim());
        }
    }
});
observer.observe(document.body, { childList: true, subtree: true, characterData: true, attributes: true });

// 2. Fallback Polling (in case it is rendered inside a textarea, canvas, or shadow-dom isolated element that we can query)
setInterval(() => {
    try {
        // Collect all text from body
        let allText = document.body.innerText || "";

        // Use a highly permissive Regex to find the last occurring valid line (to prevent decimals from breaking it)
        const regex = /Temp:\s*[-\d\.]+C\s*\|\s*Hum:\s*[\d\.]+%\s*\|\s*Soil:\s*[\d\.]+%\s*\|\s*pH:\s*[\d\.]+\s*\|\s*Light:\s*[\d\.]+%/ig;
        let matches = allText.match(regex);

        if (matches && matches.length > 0) {
            // grab the most recent one printed
            let latestLine = matches[matches.length - 1];
            processText(latestLine.trim());
        }
    } catch (e) { }
}, 1000);

console.log("[Tinkercad Bridge] Active. Polling and Observing for Serial data...");
