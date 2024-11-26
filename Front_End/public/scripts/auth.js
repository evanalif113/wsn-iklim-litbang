import { app } from "./fireconfig.js";
import { getAuth, signInWithEmailAndPassword, signOut } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-auth.js";

const auth = getAuth(app);

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

        // Set timer untuk logout otomatis setelah 1 jam
        setTimeout(() => {
            logoutUser();
            alert("You have been logged out due to inactivity.");
        }, 3600000); // 1 jam = 3600000 milidetik
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