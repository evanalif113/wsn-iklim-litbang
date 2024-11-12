// Fungsi untuk mendapatkan musim berdasarkan bulan
function getMusim(bulan) {
    if (bulan >= 10 || bulan <= 3) {
        return 'Musim Penghujan'; // Oktober hingga Maret
    } else {
        return 'Musim Kemarau'; // April hingga September
    }
}
var bulanSekarang = new Date().getMonth() + 1;
var musimSekarang = getMusim(bulanSekarang);
var infoMusimElement = document.getElementById('info-musim');
infoMusimElement.textContent = `Musim Sekarang: ${musimSekarang}`;