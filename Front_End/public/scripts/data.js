// Impor app dari fireconfig.js
import { app } from "./fireconfig.js";
import { getDatabase, ref, query, orderByKey, limitToLast, get } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-database.js";
import { getPerformance } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-performance.js";

const database = getDatabase(app);
const perf = getPerformance(app);

// Fungsi untuk menangani perubahan ID stasiun
function handleStationChange() {
    const stationSelector = document.getElementById("stationSelector");
    const selectedStation = stationSelector.value;
    loadWeatherData(selectedStation);
}

// Fungsi untuk mengambil dan menampilkan data berdasarkan ID stasiun yang dipilih
function loadWeatherData(stationId) {
    // Cek status login dari localStorage
    const isLoggedIn = localStorage.getItem('isLoggedIn');

    if (isLoggedIn !== 'true') {
        return;
    }

    const tableBody = document.getElementById("datalogger");
    tableBody.innerHTML = ""; // Kosongkan tabel sebelum mengisi data

    // Ambil data terakhir dari database sesuai ID stasiun
    const dataRef = query(ref(database, `auto_weather_stat/${stationId}/data`), orderByKey(), limitToLast(15));
    
    get(dataRef).then((snapshot) => {
        if (snapshot.exists()) {
            const dataArray = [];

            // Simpan data ke dalam array
            snapshot.forEach((childSnapshot) => {
                const data = childSnapshot.val();
                const timeFormatted = new Date(data.timestamp * 1000)
                .toLocaleString([], { 
                    year: 'numeric', 
                    month: '2-digit', 
                    day: '2-digit',
                    hour: '2-digit', 
                    minute: '2-digit', 
                    second: '2-digit',
                    hour12: false // Mengatur format 24 jam
                });
                dataArray.push({
                    time: timeFormatted,
                    ...data
                });
            });

            // Balikkan urutan data
            dataArray.reverse();

            // Tampilkan data di tabel
            dataArray.forEach((data) => {
                const row = tableBody.insertRow();
                row.insertCell(0).textContent = data.time;
                row.insertCell(1).textContent = data.temperature;
                row.insertCell(2).textContent = data.humidity;
                row.insertCell(3).textContent = data.pressure;
                row.insertCell(4).textContent = data.dew;
                row.insertCell(5).textContent = data.volt;
            });
        } else {
            console.warn("Tidak ada data yang tersedia.");
        }
    }).catch((error) => {
        console.error("Error fetching data: ", error);
    });
}

// Tambahkan event listener ke dropdown selector
document.getElementById("stationSelector").addEventListener("change", handleStationChange);

// Perbarui data setiap 1 menit (60000 milidetik)
setInterval(() => {
    const stationSelector = document.getElementById("stationSelector");
    loadWeatherData(stationSelector.value);
}, 60000);

// Bagian Pencarian
document.getElementById('searchStation').addEventListener('input', function() {
    var filter = this.value.toLowerCase();
    var options = document.getElementById('stationSelector').options;
    
    for (var i = 1; i < options.length; i++) {
        var option = options[i];
        var text = option.text.toLowerCase();
        option.style.display = text.includes(filter) ? '' : 'none';
    }
});