// chart.js
import app from './config.js';  // Impor app dari fireconfig.js
import { getDatabase, ref, query, orderByKey, limitToLast, get } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-database.js";

// Inisialisasi Realtime Database
const database = getDatabase(app);

// Maksimal panjang array data (misalnya, 60 data poin)
var maxDataLength = 60;

// Inisialisasi array kosong untuk menyimpan data
var timestamps = [];
var temperatures = [];
var humidity = [];
var pressure = [];
var dew = [];
var volt = [];

// Muat data saat halaman siap
document.addEventListener('DOMContentLoaded', () => {
    // Tambahkan event listener ke dropdown selector
    document.getElementById("stationSelector").addEventListener("change", handleStationChange);

    // Perbarui data setiap 1 menit (60000 milidetik)
    setInterval(() => {
        const stationSelector = document.getElementById("stationSelector");
        fetchLastData(stationSelector.value || 'id-02'); // Gunakan stationId dari selector atau default 'id-02'
    }, 60000);

    // Load data pertama kali menggunakan stationId default atau dari dropdown
    const initialStationId = document.getElementById("stationSelector").value || 'id-02';
    fetchLastData(initialStationId);
});

// Fungsi untuk menangani perubahan ID stasiun
function handleStationChange() {
    const stationSelector = document.getElementById("stationSelector");
    const selectedStation = stationSelector.value;
    fetchLastData(selectedStation);
}

// Fungsi untuk mengambil data terbaru menggunakan Firebase Realtime Database
function fetchLastData(stationId) {
    var hour = 1;
    var fetchCount = hour * 60;
    var dataRef = query(ref(database, `auto_weather_stat/${stationId}/data`), orderByKey(), limitToLast(fetchCount));

    get(dataRef).then((snapshot) => {
        if (snapshot.exists()) {
            const data = snapshot.val();
            timestamps = [];
            temperatures = [];
            humidity = [];
            pressure = [];
            dew = [];
            volt = [];

            Object.values(data).forEach(entry => {
                // Konversi timestamp ke waktu terformat
                var timeFormatted = new Date(entry.timestamp * 1000)
                                    .toLocaleTimeString([], { 
                                        hour: '2-digit', 
                                        minute: '2-digit', 
                                        second: '2-digit', 
                                        hour12: false 
                                    });

                // Update array dengan data baru (geser array ketika data melebihi panjang maksimum)
                updateDataArray(timestamps, timeFormatted);
                updateDataArray(temperatures, entry.temperature);
                updateDataArray(humidity, entry.humidity);
                updateDataArray(pressure, entry.pressure);
                updateDataArray(dew, entry.dew);
                updateDataArray(volt, entry.volt);
            });

            // Update chart dengan data baru
            updateCharts();
        } else {
            console.warn("Tidak ada data yang tersedia.");
        }
    }).catch((error) => {
        console.error("Error fetching data: ", error);
    });
}

// Fungsi untuk mengupdate array data dengan panjang maksimal 60
function updateDataArray(array, newValue) {
    if (array.length >= maxDataLength) {
        array.shift();  // Hapus elemen pertama
    }
    array.push(newValue);  // Tambahkan nilai baru di akhir
}

// Fungsi untuk menampilkan chart suhu
function plotTemperatureChart() {
    var trace = {
        x: timestamps,
        y: temperatures,
        type: 'scatter',
        mode: 'lines+markers',
        name: 'Suhu Lingkungan (°C)',
        line: { color: '#C70039' }
    };

    var layout = {
        title: 'Suhu Lingkungan (°C)',
        xaxis: { title: 'Waktu' },
        yaxis: { title: 'Suhu Lingkungan (°C)' },
        height: 400
    };

    Plotly.newPlot('temperature-chart', [trace], layout);
}

// Fungsi untuk menampilkan chart kelembapan
function plotHumidityChart() {
    var trace = {
        x: timestamps,
        y: humidity,
        type: 'scatter',
        mode: 'lines+markers',
        name: 'Kelembapan Relatif (%)',
        line: { color: '006BFF' }
    };

    var layout = {
        title: 'Kelembapan Relatif (%)',
        xaxis: { title: 'Waktu' },
        yaxis: { title: 'Kelembapan Relatif (%)' },
        height: 400
    };

    Plotly.newPlot('humidity-chart', [trace], layout);
}

// Fungsi untuk menampilkan chart titik embun
function plotDewChart() {
    var trace = {
        x: timestamps,
        y: dew,
        type: 'scatter',
        mode: 'lines+markers',
        name: 'Titik Embun (°C)',
        line: { color: '4F75FF' }
    };

    var layout = {
        title: 'Titik Embun (°C)',
        xaxis: { title: 'Waktu' },
        yaxis: { title: 'Titik Embun (°C)' },
        height: 400
    };

    Plotly.newPlot('dew-chart', [trace], layout);
}

// Fungsi untuk menampilkan chart tekanan
function plotPressureChart() {
    var trace = {
        x: timestamps,
        y: pressure,
        type: 'scatter',
        mode: 'lines+markers',
        name: 'Tekanan Udara (hPa)',
        line: { color: '15B392' }
    };

    var layout = {
        title: 'Tekanan Udara (hPa)',
        xaxis: { title: 'Waktu' },
        yaxis: { title: 'Tekanan Udara (hPa)' },
        height: 400
    };

    Plotly.newPlot('pressure-chart', [trace], layout);
}

function plotVoltChart() {
    var trace = {
        x: timestamps,
        y: volt,
        type: 'scatter',
        mode: 'lines+markers',
        name: 'Tegangan (V)',
        line: { color: 'FFC300' }
    };

    var layout = {
        title: 'Tegangan (V)',
        xaxis: { title: 'Waktu' },
        yaxis: { title: 'Tegangan (V)' },
        height: 400
    };

    Plotly.newPlot('volt-chart', [trace], layout);
}

function plotTempHumiScatter() {
    var trace1 = {
        x: temperatures,
        y: humidity,
        mode: 'markers',
        type: 'scatter',
        marker: { color: '#15B392' }
    };

    var layout = {
        title: 'Scatter Suhu dan Kelembapan',
        xaxis: { title: 'Kelembapan' },
        yaxis: { title: 'Suhu' },
        height: 400
    };

    var data = [trace1];
    Plotly.newPlot('scatter-chart', data, layout);
}

function plotStacked() {
    var trace1 = {
        x: timestamps,
        y: temperatures,
        type: 'scatter',
        mode: 'markers',
        name: 'Suhu'
      };
      
      var trace2 = {
        x: timestamps,
        y: dew,
        type: 'scatter',
        mode: 'markers',
        name: 'Titik Embun'
      };
      
      var data = [trace1, trace2];
      
      var layout = {
        title: 'Scatter Suhu dan Titik Embun',
        xaxis: { title: 'Waktu' },
        yaxis: { title: 'Suhu' },
        height: 400
    };
      
      Plotly.newPlot('stacked-chart', data, layout);
}

// Fungsi untuk mengupdate chart secara dinamis
function updateCharts() {
    var data_update_temp = { 
        x: [timestamps], 
        y: [temperatures] 
    };
    Plotly.update('temperature-chart', data_update_temp);

    var data_update_humid = { 
        x: [timestamps], 
        y: [humidity] 
    };
    Plotly.update('humidity-chart', data_update_humid);

    var data_update_press = { 
        x: [timestamps], 
        y: [pressure] 
    };
    Plotly.update('pressure-chart', data_update_press);

    var data_update_dew = { 
        x: [timestamps], 
        y: [dew] 
    };
    Plotly.update('dew-chart', data_update_dew);

    var data_update_volt = { 
        x: [timestamps], 
        y: [volt] 
    };
    Plotly.update('volt-chart', data_update_volt);

    var data_update_scatter = { 
        x: [temperatures],
        y: [humidity]
    };
    Plotly.update('scatter-chart', data_update_scatter);

    var data_update_stacked = { 
        x: [timestamps],
        y: [temperatures, dew]
    };
    Plotly.update('stacked-chart', data_update_stacked);
}

// Fetch data initially and set intervals for every minute
$(document).ready(function() {
    plotTemperatureChart();
    plotHumidityChart();
    plotPressureChart();
    plotDewChart();
    plotVoltChart();
    plotTempHumiScatter();
    plotStacked();

    fetchLastData();
    setInterval(fetchLastData, 30000); // Fetch new data every 30 seconds
});

