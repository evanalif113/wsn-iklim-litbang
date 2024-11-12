// Replace these coordinates with the actual latitude and longitude of your station
var BalaiDesaKoordinat = [-7.620403, 109.644307];
var StaKlim1Koordinat = [-7.62306333, 109.64390667];
var StaKlim2Koordinat = [-7.62046667, 109.6445233];
var StaKlim3Koordinat = [-7.62046667, 109.6445233];


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
L.marker(BalaiDesaKoordinat, { icon: HomeIcon })
    .addTo(map)
    .bindPopup('Lokasi Kampung Iklim Desa Prigi <br>Latitude: ' + BalaiDesaKoordinat[0] + '<br>Longitude: ' + BalaiDesaKoordinat[1])
    .openPopup();



    /*
// Penanada Stasiun Iklim 1
L.marker(StaKlim1Koordinat, { icon: HomeIcon })
    .addTo(map)
    .bindPopup('Lokasi Stasiun Iklim 1 <br>Latitude: ' + StaKlim1Koordinat[0] + '<br>Longitude: ' + StaKlim1Koordinat[1])
    .openPopup();

// Penanda Stasiun Iklim 2
L.marker(StaKlim2Koordinat, { icon: HomeIcon })
    .addTo(map)
    .bindPopup('Lokasi Stasiun Iklim 2 <br>Latitude: ' + StaKlim2Koordinat[0] + '<br>Longitude: ' + StaKlim2Koordinat[1])
    .openPopup();

// Penanda Stasiun Iklim 3
L.marker(StaKlim3Koordinat, { icon: HomeIcon })
    .addTo(map)
    .bindPopup('Lokasi Stasiun Iklim 2 <br>Latitude: ' + StaKlim3Koordinat[0] + '<br>Longitude: ' + StaKlim2Koordinat[1])
    .openPopup(); */