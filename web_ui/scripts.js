async function updateEsp32Time() {

    try {

        const response = await fetch('/api/status');

        if (!response.ok) {
            throw new Error('HTTP error: ' + response.status);
        }

        const data = await response.json();
        
        document.getElementById('esp32_time').textContent =
            data.esp32_time;

        document.getElementById('wifi_status').textContent =
            data.wifi_status ? 'Connected' : 'Disconnected';

        document.getElementById('load_status').textContent =
            data.load_status ? 'ON' : 'OFF';

    } catch (error) {

        console.log('Error fetching ESP32 status:', error);

    }
}


async function controlGeyser(cmd) {

    try {

        const response = await fetch('/api/geyser', {
            method: 'POST',

            headers: {
                'Content-Type': 'application/json'
            },

            body: JSON.stringify({
                state: cmd
            })
        });

        if (!response.ok) {
            throw new Error('HTTP error: ' + response.status);
        }

        const data = await response.json();

        document.getElementById('command_result').textContent =
            data.message;

        updateEsp32Time();

    } catch (error) {

        document.getElementById('command_result').textContent =
            'Command failed: ' + error.message;

        console.log('Geyser command error:', error);

    }
}


updateEsp32Time();

setInterval(updateEsp32Time, 5000);