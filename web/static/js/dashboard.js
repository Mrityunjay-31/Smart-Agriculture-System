document.addEventListener('DOMContentLoaded', () => {
    // Selectors
    const els = {
        userName: document.getElementById('userName'),
        userInitial: document.getElementById('userInitial'),

        lastUpdate: document.getElementById('lastUpdateSpan'),
        sysDot: document.getElementById('systemStatusDot'),
        ardDot: document.getElementById('arduinoStatusDot'),

        tempVal: document.getElementById('tempValue'),
        tempStat: document.getElementById('tempStatus'),

        humVal: document.getElementById('humValue'),
        humStat: document.getElementById('humStatus'),

        soilVal: document.getElementById('soilValue'),
        soilStat: document.getElementById('soilStatus'),

        phVal: document.getElementById('phValue'),
        phStat: document.getElementById('phStatus'),

        lightVal: document.getElementById('lightValue'),
        lightStat: document.getElementById('lightStatus'),
    };

    // Fetch Profile
    const fetchProfile = async () => {
        try {
            const res = await fetch('/api/auth/session');
            if (res.ok) {
                const data = await res.json();
                if (data.name) {
                    els.userName.textContent = data.name;
                    els.userInitial.textContent = data.name.charAt(0).toUpperCase();
                }
            }
        } catch (e) { console.error('Error fetching session:', e); }
    };

    // Fetch System Status
    const fetchSystemStatus = async () => {
        try {
            const res = await fetch('/api/system/status');
            if (res.ok) {
                const data = await res.json();
                // Update Pipeline DOT
                els.sysDot.className = 'status-dot ' + (data.firebaseConnected ? 'active' : 'error');

                const labelEl = document.getElementById('arduinoStatusLabel');
                if (data.mode === 'tinkercad') {
                    if (labelEl) labelEl.textContent = 'Sensor Source: Tinkercad Simulation';
                    els.ardDot.className = 'status-dot ' + (data.arduinoStatus === 'CONNECTED' ? 'active' : 'error');
                } else if (data.mode === 'simulation') {
                    if (labelEl) labelEl.textContent = 'Sensor Source: Simulation';
                    els.ardDot.className = 'status-dot active';
                } else {
                    if (labelEl) labelEl.textContent = 'Sensor Source: Arduino';
                    els.ardDot.className = 'status-dot ' + (data.arduinoStatus === 'CONNECTED' ? 'active' : 'error');
                }
            }
        } catch (e) {
            els.sysDot.className = 'status-dot error';
            els.ardDot.className = 'status-dot error';
        }
    };

    // Fetch Latest Sensors
    const fetchSensors = async () => {
        try {
            const res = await fetch('/api/sensors/latest');
            if (res.ok) {
                const data = await res.json();

                // Update values
                els.tempVal.textContent = data.temperature.toFixed(1);
                els.humVal.textContent = data.humidity.toFixed(1);
                els.soilVal.textContent = data.soilMoisture.toFixed(1);
                els.phVal.textContent = data.ph.toFixed(1);
                els.lightVal.textContent = data.lightIntensity.toFixed(0);

                // Update statuses & colors
                updateStatus(els.tempStat, data.temperatureStatus);
                updateStatus(els.humStat, data.humidityStatus);
                updateStatus(els.soilStat, data.soilStatus);
                updateStatus(els.phStat, data.phStatus);
                updateStatus(els.lightStat, data.lightStatus);

                // Update timestamp
                const date = new Date(data.timestamp);
                els.lastUpdate.textContent = date.toLocaleTimeString();
            }
        } catch (e) {
            console.error('Error fetching sensor data:', e);
        }
    };

    const updateStatus = (el, statusObj) => {
        const val = typeof statusObj === 'string' ? statusObj.toUpperCase() : "OPTIMAL";
        el.textContent = val;
        if (val === 'CRITICAL' || val === 'DANGER') {
            el.className = 'card-status status-critical';
        } else if (val === 'WARNING') {
            el.className = 'card-status status-warning';
        } else {
            el.className = 'card-status status-optimal';
        }
    };

    // Init
    fetchProfile();
    fetchSystemStatus();
    fetchSensors();



    // Polling Intervals
    setInterval(fetchSystemStatus, 5000);
    setInterval(fetchSensors, 2000); // Poll every 2s in local setup
});
