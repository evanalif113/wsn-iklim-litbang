// Impor app dari fireconfig.js
import { initializeApp } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-app.js";
import { getAnalytics } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-analytics.js";
import { getDatabase, ref, query, orderByKey, limitToLast, get } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-database.js";

// Konfigurasi Firebase
const firebaseConfig = {
    apiKey: "AIzaSyDPoeHnDs1E4BxAMDoLEub2uE9q6H_YDw4",
    authDomain: "database-sensor-iklim-litbang.firebaseapp.com",
    databaseURL: "https://database-sensor-iklim-litbang-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "database-sensor-iklim-litbang",
    storageBucket: "database-sensor-iklim-litbang.appspot.com",
    messagingSenderId: "849206798206",
    appId: "1:849206798206:web:7fe6fe938389658302752f",
    measurementId: "G-GH86DQR6NJ"
};

// Inisialisasi Firebase App
const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);
// Inisialisasi Realtime Database
const database = getDatabase(app);

// Fungsi untuk mengambil dan menampilkan data berdasarkan ID stasiun yang dipilih
function loadWeatherData(stationId = 'id-02') {
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
                                        .toLocaleTimeString([], { 
                                            year: 'numeric', 
                                            month: '2-digit', 
                                            day: '2-digit', 
                                            hour: '2-digit', 
                                            minute: '2-digit', 
                                            second: '2-digit', 
                                            hour12: false
                                        });
                dataArray.push({
                    date: timeFormatted,
                    temperature: data.temperature,
                    humidity: data.humidity,
                    pressure: data.pressure,
                    dew: data.dew,
                    volt: data.volt
                });
            });

            // Balik array agar data terbaru di atas
            dataArray.reverse();

            // Tambahkan data ke tabel
            dataArray.forEach(entry => {
                const row = document.createElement("tr");
                row.innerHTML = `
                    <td>${entry.date}</td>
                    <td>${entry.temperature}</td>
                    <td>${entry.humidity}</td>
                    <td>${entry.pressure}</td>
                    <td>${entry.dew}</td>
                    <td>${entry.volt}</td>
                `;
                tableBody.appendChild(row);
            });
        } else {
            console.warn("Tidak ada data yang tersedia.");
        }
    }).catch((error) => {
        console.error("Error fetching data: ", error);
    });
}

// Fungsi untuk menangani perubahan ID stasiun
function handleStationChange() {
    const stationSelector = document.getElementById("stationSelector");
    const selectedStation = stationSelector.value;
    loadWeatherData(selectedStation);
}

// Muat data saat halaman siap
document.addEventListener('DOMContentLoaded', () => {
    // Panggil loadWeatherData pertama kali dengan stasiun default
    loadWeatherData();

    // Tambahkan event listener ke dropdown selector
    document.getElementById("stationSelector").addEventListener("change", handleStationChange);

    // Perbarui data setiap 1 menit (60000 milidetik)
    setInterval(() => {
        const stationSelector = document.getElementById("stationSelector");
        loadWeatherData(stationSelector.value || 'id-02');
    }, 60000);
});

document.getElementById('searchStation').addEventListener('input', function() {
    var filter = this.value.toLowerCase();
    var options = document.getElementById('stationSelector').options;
    
    for (var i = 1; i < options.length; i++) {
        var option = options[i];
        var text = option.text.toLowerCase();
        option.style.display = text.includes(filter) ? '' : 'none';
    }
});
