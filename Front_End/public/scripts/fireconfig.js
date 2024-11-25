// fireconfig.js
import { initializeApp } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-app.js";
import { getAuth } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-auth.js";
import { getDatabase } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-database.js";

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

// Ekspor variabel
export { app, auth, database };
