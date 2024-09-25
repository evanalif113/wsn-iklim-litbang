// Fungsi untuk mendapatkan musim berdasarkan bulan
function getMusim(bulan) {
    if (bulan >= 10 || bulan <= 3) {
        return 'Musim Penghujan'; // Oktober hingga Maret
    } else {
        return 'Musim Kemarau'; // April hingga September
    }
}

// Ambil bulan saat ini
var bulanSekarang = new Date().getMonth() + 1;

// Tentukan musim
var musimSekarang = getMusim(bulanSekarang);

// Tampilkan informasi musim pada elemen dengan ID 'info-musim'
var infoMusimElement = document.getElementById('info-musim');
infoMusimElement.textContent = `Musim Sekarang: ${musimSekarang}`;


// mengambil data
function hitungRataRataDataThingspeak(channelID, fieldNumber, numResults, resultElementId) {
    const apiKey = '6NPQMVGO26LY3UCP'; //API Key Thingspeak User
    const url = `https://api.thingspeak.com/channels/${channelID}/fields/${fieldNumber}.json?api_key=${'6NPQMVGO26LY3UCP'}&results=${numResults}`;

    fetch(url)
        .then((response) => response.json())
        .then((data) => {
            if (data.feeds && data.feeds.length > 0) {
                const fieldValues = data.feeds.map((feed) => parseFloat(feed[`field${fieldNumber}`]));
                const rataRata = fieldValues.reduce((total, value) => total + value, 0) / fieldValues.length;

                // Tampilkan rata-rata data
                const rataRataElement = document.getElementById(resultElementId);
                rataRataElement.textContent = `Hasil ${resultElementId}: ${rataRata.toFixed(2)}`;
            }
        })
        .catch((error) => {
            console.error('Gagal mengambil data Thingspeak:', error);
        });
}

// panggil rata rata
hitungRataRataDataThingspeak(2281820, 1, 60, 'rata-rata-suhu1');
hitungRataRataDataThingspeak(2281820, 2, 60, 'rata-rata-kelembaban1');
hitungRataRataDataThingspeak(2281820, 3, 60, 'rata-rata-tekanan1');
hitungRataRataDataThingspeak(2289583, 1, 60, 'rata-rata-suhu2');
hitungRataRataDataThingspeak(2289583, 2, 60, 'rata-rata-kelembaban2');
hitungRataRataDataThingspeak(2289583, 3, 60, 'rata-rata-tekanan2');
hitungRataRataDataThingspeak(2326256, 1, 60, 'rata-rata-suhu3');
hitungRataRataDataThingspeak(2326256, 2, 60, 'rata-rata-kelembaban3');
hitungRataRataDataThingspeak(2326256, 3, 60, 'rata-rata-tekanan3');



// Replace these coordinates with the actual latitude and longitude of your station
var BalaiDesaKoordinat = [-7.620403, 109.644307];
var StaKlim1Koordinat = [-7.62306333, 109.64390667];
var StaKlim2Koordinat = [-7.62046667, 109.6445233];


// Initialize the map
var map = L.map('map').setView(BalaiDesaKoordinat, 14);

// Add a tile layer (you can choose a different provider)
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '© OpenStreetMap contributors'
}).addTo(map);
// Add custom marker
var HomeIcon = L.icon({
    iconUrl: './img/icon.png',
    iconSize: [32, 32], // width and height of the icon
    iconAnchor: [16, 32], // point of the icon that corresponds to the marker's location
    popupAnchor: [0, -32] // point from which the popup should open relative to the iconAnchor
    //shadowUrl: './img/icon.png',
    //iconSize:     [38, 95], // size of the icon
    //shadowSize:   [50, 64], // size of the shadow
    //iconAnchor:   [22, 94], // point of the icon which will correspond to marker's location
    //shadowAnchor: [4, 62],  // the same for the shadow
    //popupAnchor:  [-3, -76] // point from which the popup should open relative to the iconAnchor
});
// Penanda Balai Desa
L.marker(BalaiDesaKoordinat, { icon: HomeIcon }).addTo(map)
    .bindPopup('Lokasi Kampung Iklim Desa Prigi <br>Latitude: ' + BalaiDesaKoordinat[0] + '<br>Longitude: ' + BalaiDesaKoordinat[1])
    .openPopup();

// Penanada Stasiun Iklim 1
L.marker(StaKlim1Koordinat, { icon: HomeIcon }).addTo(map)
    .bindPopup('Lokasi Stasiun Iklim 1 <br>Latitude: ' + StaKlim1Koordinat[0] + '<br>Longitude: ' + StaKlim1Koordinat[1])
    .openPopup();

// Penanda Stasiun Iklim 2
L.marker(StaKlim2Koordinat, { icon: HomeIcon }).addTo(map)
    .bindPopup('Lokasi Stasiun Iklim 2 <br>Latitude: ' + StaKlim2Koordinat[0] + '<br>Longitude: ' + StaKlim2Koordinat[1])
    .openPopup();