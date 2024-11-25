// Impor app dari fireconfig.js
import { initializeApp } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-app.js";
import { getAuth, signInWithEmailAndPassword, signOut } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-auth.js";
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
const auth = getAuth(app);
const database = getDatabase(app);

// Fungsi untuk autentikasi pengguna
function authenticateUser(email, password) {
    signInWithEmailAndPassword(auth, email, password)
        .then((userCredential) => {
            console.log("User signed in:", userCredential.user);
            const uid = userCredential.user.uid;
            console.log("User UID:", uid);

            // Simpan status login ke localStorage
            localStorage.setItem('isLoggedIn', 'true');

            // Tutup modal setelah login
            var loginModal = bootstrap.Modal.getInstance(document.getElementById('loginModal'));
            loginModal.hide();

            // Tampilkan tombol logout dan sembunyikan tombol login
            document.getElementById('loginButton').style.display = 'none';
            document.getElementById('logoutButton').style.display = 'block';
        })
        .catch((error) => {
            console.error("Error signing in:", error);
            alert("Error signing in: " + error.message);

            // Tampilkan kembali modal login jika autentikasi gagal
            var loginModal = new bootstrap.Modal(document.getElementById('loginModal'));
            loginModal.show();
        });
}

// Fungsi untuk logout pengguna
function logoutUser() {
    signOut(auth).then(() => {
        console.log("User signed out");

        // Hapus status login dari localStorage
        localStorage.removeItem('isLoggedIn');

        // Tampilkan tombol login dan sembunyikan tombol logout
        document.getElementById('loginButton').style.display = 'block';
        document.getElementById('logoutButton').style.display = 'none';

        // Tampilkan modal login setelah logout
        const loginModal = new bootstrap.Modal(document.getElementById('loginModal'));
        loginModal.show();
    }).catch((error) => {
        console.error("Error signing out:", error);
    });
}

// Periksa status login saat halaman dimuat
window.addEventListener('load', function() {
    const isLoggedIn = localStorage.getItem('isLoggedIn');
    if (!isLoggedIn) {
        // Tampilkan modal login jika belum login
        const loginModal = new bootstrap.Modal(document.getElementById('loginModal'));
        loginModal.show();
    } else {
        // Tampilkan tombol logout dan sembunyikan tombol login
        document.getElementById('loginButton').style.display = 'none';
        document.getElementById('logoutButton').style.display = 'block';
    }
});

// Tambahkan event listener untuk form login
document.getElementById('loginForm').addEventListener('submit', function(event) {
    event.preventDefault();
    
    const email = document.getElementById('email').value;
    const password = document.getElementById('password').value;

    // Panggil fungsi autentikasi
    authenticateUser(email, password);
});

// Tambahkan event listener untuk tombol logout
document.getElementById('logoutButton').addEventListener('click', function() {
    logoutUser();
});

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