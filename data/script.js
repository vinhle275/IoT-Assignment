// ==================== SIMPLE POLLING SYSTEM ====================
let tempChart, humChart;
let chartData = {
    labels: [],
    temps: [],
    hums: []
};
const maxDataPoints = 30;

// Start polling when page loads
document.addEventListener('DOMContentLoaded', function() {
    console.log('✅ Page loaded, starting sensor polling...');
    
    // Initialize charts if Chart.js exists
    if (typeof Chart !== 'undefined') {
        initCharts();
    }
    
    // Start polling data every 2 seconds
    fetchSensorData();
    setInterval(fetchSensorData, 2000);
});

// Fetch sensor data from /sensors endpoint
function fetchSensorData() {
    fetch('/sensors')
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            console.log('📊 Sensor data:', data);
            updateUI(data);
            updateCharts(data);
        })
        .catch(error => {
            console.error('❌ Fetch error:', error);
            const statusEl = document.getElementById('status');
            if (statusEl) {
                statusEl.textContent = '❌ Connection Error';
                statusEl.style.color = '#F44336';
            }
        });
}

// Update UI with sensor data
function updateUI(data) {
    // Update temperature
    if (data.temp !== undefined) {
        const tempElement = document.getElementById('temp');
        if (tempElement) {
            tempElement.textContent = data.temp.toFixed(1);
        }
        
        // Update temperature color status
        const tempStatus = document.getElementById('tempStatus');
        if (tempStatus) {
            if (data.temp < 15) {
                tempStatus.textContent = '❄️ Cold';
                tempStatus.style.color = '#2196F3';
            } else if (data.temp < 25) {
                tempStatus.textContent = '✅ Normal';
                tempStatus.style.color = '#4CAF50';
            } else if (data.temp < 35) {
                tempStatus.textContent = '⚠️ Warm';
                tempStatus.style.color = '#FF9800';
            } else {
                tempStatus.textContent = '🔥 Hot';
                tempStatus.style.color = '#F44336';
            }
        }
    }
    
    // Update humidity
    if (data.hum !== undefined) {
        const humElement = document.getElementById('hum');
        if (humElement) {
            humElement.textContent = data.hum.toFixed(1);
        }
        
        // Update humidity color status
        const humStatus = document.getElementById('humStatus');
        if (humStatus) {
            if (data.hum < 30) {
                humStatus.textContent = '🏜️ Dry';
                humStatus.style.color = '#FF9800';
            } else if (data.hum < 70) {
                humStatus.textContent = '✅ Normal';
                humStatus.style.color = '#4CAF50';
            } else {
                humStatus.textContent = '💦 Humid';
                humStatus.style.color = '#2196F3';
            }
        }
    }
    
    // Update connection status
    const statusElement = document.getElementById('status');
    if (statusElement) {
        statusElement.textContent = '✅ Connected';
        statusElement.style.color = '#4CAF50';
    }
}

// Initialize charts
function initCharts() {
    try {
        const tempCtx = document.getElementById('tempChart');
        const humCtx = document.getElementById('humChart');
        
        if (tempCtx) {
            tempChart = new Chart(tempCtx.getContext('2d'), {
                type: 'line',
                data: {
                    labels: chartData.labels,
                    datasets: [{
                        label: 'Temperature (°C)',
                        data: chartData.temps,
                        borderColor: '#FF6B6B',
                        backgroundColor: 'rgba(255, 107, 107, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true,
                        pointRadius: 4,
                        pointHoverRadius: 6
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { display: true }
                    },
                    scales: {
                        y: {
                            min: 15,
                            max: 40,
                            beginAtZero: false
                        }
                    }
                }
            });
        }
        
        if (humCtx) {
            humChart = new Chart(humCtx.getContext('2d'), {
                type: 'line',
                data: {
                    labels: chartData.labels,
                    datasets: [{
                        label: 'Humidity (%)',
                        data: chartData.hums,
                        borderColor: '#42A5F5',
                        backgroundColor: 'rgba(66, 165, 245, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true,
                        pointRadius: 4,
                        pointHoverRadius: 6
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { display: true }
                    },
                    scales: {
                        y: {
                            min: 0,
                            max: 100,
                            beginAtZero: false
                        }
                    }
                }
            });
        }
    } catch (error) {
        console.warn('⚠️ Chart initialization error:', error);
    }
}

// Update chart data
function updateCharts(data) {
    if (!tempChart || !humChart) return;
    
    // Add current time label
    const now = new Date();
    const timeLabel = now.getHours().toString().padStart(2, '0') + ':' + 
                      now.getMinutes().toString().padStart(2, '0') + ':' +
                      now.getSeconds().toString().padStart(2, '0');
    
    chartData.labels.push(timeLabel);
    chartData.temps.push(data.temp || 0);
    chartData.hums.push(data.hum || 0);
    
    // Keep only last N points
    if (chartData.labels.length > maxDataPoints) {
        chartData.labels.shift();
        chartData.temps.shift();
        chartData.hums.shift();
    }
    
    // Update charts
    tempChart.data.labels = chartData.labels;
    tempChart.data.datasets[0].data = chartData.temps;
    tempChart.update('none');
    
    humChart.data.labels = chartData.labels;
    humChart.data.datasets[0].data = chartData.hums;
    humChart.update('none');
}

// Toggle LED
function toggleLED(ledNum) {
    fetch(`/toggle?led=${ledNum}`)
        .then(r => r.json())
        .then(data => {
            console.log('LED toggled:', data);
            const btn = document.querySelector(`button[onclick="toggleLED(${ledNum})"]`);
            if (btn) {
                btn.style.background = data[`led${ledNum}`] === 'ON' ? '#4CAF50' : '#ccc';
            }
        })
        .catch(e => console.error('Toggle error:', e));
}

// WiFi scan
function scanWiFi() {
    const btn = document.querySelector('button[onclick="scanWiFi()"]');
    if (btn) btn.textContent = '🔄 Scanning...';
    
    fetch('/scan')
        .then(r => r.json())
        .then(networks => {
            console.log('Networks found:', networks);
            const options = networks.map((net, idx) => 
                `<option value="${net.ssid}">${net.ssid} (${net.rssi})</option>`
            ).join('');
            const ssidSel = document.getElementById('ssid');
            if (ssidSel) {
                ssidSel.innerHTML = `<option>-- Select WiFi --</option>${options}`;
            }
        })
        .catch(e => console.error('Scan error:', e))
        .finally(() => {
            if (btn) btn.textContent = '🔍 Scan WiFi';
        });
}

// Connect WiFi
function connectWiFi(event) {
    event.preventDefault();
    const ssidEl = document.getElementById('ssid');
    const passEl = document.getElementById('pass');
    const ssid = ssidEl ? ssidEl.value : '';
    const pass = passEl ? passEl.value : '';
    
    if (!ssid) {
        alert('⚠️ Please select SSID');
        return;
    }
    
    const url = `/connect?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`;
    fetch(url)
        .then(r => r.text())
        .then(msg => {
            alert('✅ Connecting to WiFi... Please wait 10 seconds');
            setTimeout(() => {
                window.location.reload();
            }, 5000);
        })
        .catch(e => {
            alert('❌ Connection error: ' + e);
        });
}

// Reset to AP mode
function resetToAP() {
    if (!confirm('❓ Reset to AP mode?')) return;
    
    fetch('/resetAP')
        .then(r => r.text())
        .then(msg => {
            alert('✅ Resetting... Redirecting to AP mode');
            setTimeout(() => {
                window.location = '/';
            }, 2000);
        })
        .catch(e => console.error('Reset error:', e));
}
