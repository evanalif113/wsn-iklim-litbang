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

// Mengambil data
function hitungRataRataDataThingspeak(channelID, fieldNumber, numResults, resultElementId) {
    const apiKey = '6NPQMVGO26LY3UCP'; // API Key Thingspeak User
    const url = `https://api.thingspeak.com/channels/${channelID}/fields/${fieldNumber}.json?api_key=${apiKey}&results=${numResults}`;

    fetch(url)
        .then((response) => response.json())
        .then((data) => {
            if (data.feeds && data.feeds.length > 0) {
                const fieldValues = data.feeds.map((feed) => parseFloat(feed[`field${fieldNumber}`]));
                
                // Filter out invalid or NaN values
                const validFieldValues = fieldValues.filter(value => !isNaN(value));
                const rataRata = validFieldValues.reduce((total, value) => total + value, 0) / validFieldValues.length;

                // Tentukan tipe data berdasarkan fieldNumber
                let tipeData = '';
                switch (fieldNumber) {
                    case 1:
                        tipeData = 'Suhu';
                        break;
                    case 2:
                        tipeData = 'Kelembapan';
                        break;
                    case 3:
                        tipeData = 'Tekanan Udara';
                        break;
                    default:
                        tipeData = 'Data';
                }

                // Tampilkan rata-rata data
                const rataRataElement = document.getElementById(resultElementId);
                if (rataRataElement) {
                    rataRataElement.textContent = `Hasil ${tipeData}: ${rataRata.toFixed(2)}`;
                } else {
                    console.error(`Elemen dengan ID "${resultElementId}" tidak ditemukan.`);
                }
            } else {
                console.warn(`Tidak ada data ditemukan untuk field ${fieldNumber} pada channel ${channelID}.`);
            }
        })
        .catch((error) => {
            console.error('Gagal mengambil data Thingspeak:', error);
        });
}

// Panggil fungsi hitungRataRataDataThingspeak untuk masing-masing channel dan field
hitungRataRataDataThingspeak(2281820, 1, 60, 'suhu1');
hitungRataRataDataThingspeak(2281820, 2, 60, 'kelembaban1');
hitungRataRataDataThingspeak(2281820, 3, 60, 'tekanan1');
hitungRataRataDataThingspeak(2289583, 1, 60, 'suhu2');
hitungRataRataDataThingspeak(2289583, 2, 60, 'kelembaban2');
hitungRataRataDataThingspeak(2289583, 3, 60, 'tekanan2');
hitungRataRataDataThingspeak(2326256, 1, 60, 'suhu3');
hitungRataRataDataThingspeak(2326256, 2, 60, 'kelembaban3');
hitungRataRataDataThingspeak(2326256, 3, 60, 'tekanan3');




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