/**
 * @file WebserverHTML.h
 * @brief HTML page for ESP32 web server displaying sensor data.
 *
 * This file contains the HTML, CSS, and JavaScript code that the ESP32 serves to clients.
 * The page dynamically updates sensor values and alerts using JavaScript and fetch requests.
 */

const char PAGE_MAIN[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Sensor Data</title>
    <style>
        /* Page styling */
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f4f4f4;
            margin: 0;
            padding: 50px;
        }
        h1 {
            color: #333;
        }
        .data-container {
            font-size: 1.5em;
            font-weight: bold;
            color: #555;
            margin: 20px 0;
        }
        .label {
            font-weight: bold;
            color: #222;
        }
        .alert-box {
            display: none;
            color: white;
            background-color: red;
            padding: 15px;
            font-size: 1.2em;
            font-weight: bold;
            margin: 20px;
            border-radius: 5px;
        }
    </style>
    <script>
        /**
         * @brief Fetches updated sensor data from the server and updates the page.
         */
        function fetchData() {
            fetch("/")
            .then(response => response.text())
            .then(html => {
                let parser = new DOMParser();
                let doc = parser.parseFromString(html, "text/html");

                document.getElementById("temperature").innerText = doc.getElementById("temperature").innerText;
                document.getElementById("distance").innerText = doc.getElementById("distance").innerText;
                
                let alertBox = document.getElementById("alertBox");
                let newAlertMessage = doc.getElementById("alertMessage").innerText;

                if (newAlertMessage.trim() !== "") {
                    alertBox.innerText = newAlertMessage;
                    alertBox.style.display = "block";
                } else {
                    alertBox.style.display = "none";
                }
            })
            .catch(error => console.error("Error fetching data:", error));
        }

        // Update sensor data every second
        setInterval(fetchData, 1000);
    </script>
</head>
<body>
    <h1>ESP32 Sensor Data</h1>

    <!-- Alert message box -->
    <div class="alert-box" id="alertBox"></div>

    <!-- Display sensor data -->
    <div class="data-container">
        <p><span class="label">Temperature:</span> <span id="temperature">{{temperature}}</span> °F</p>
        <p><span class="label">Distance:</span> <span id="distance">{{distance}}</span> cm</p>
    </div>

    <!-- Hidden alert message for script usage -->
    <div id="alertMessage" style="display: none;">{{alertMessage}}</div>
</body>
</html>

)=====";
