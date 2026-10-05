document.addEventListener('DOMContentLoaded', () => {
    const tokenInput = document.getElementById('tokenInput');
    const saveBtn = document.getElementById('saveBtn');
    const status = document.getElementById('status');

    // Load existing
    chrome.storage.sync.get(['bridgeToken'], function (result) {
        if (result.bridgeToken) {
            tokenInput.value = result.bridgeToken;
        }
    });

    saveBtn.addEventListener('click', () => {
        const val = tokenInput.value.trim();
        chrome.storage.sync.set({ bridgeToken: val }, function () {
            status.style.display = 'block';
            setTimeout(() => { status.style.display = 'none'; }, 2000);
        });
    });
});
