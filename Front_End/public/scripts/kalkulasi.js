// Mengambil data
function hitungRerata(channelID, fieldNumber, numResults, resultElementId) {
    var apiKey = '6NPQMVGO26LY3UCP'; // API Key Thingspeak User
    var url = `https://api.thingspeak.com/channels/${channelID}/fields/${fieldNumber}.json?api_key=${apiKey}&results=${numResults}`;

    fetch(url)
        .then((response) => response.json())
        .then((data) => {
            if (data.feeds && data.feeds.length > 0) {
                var fieldValues = data.feeds.map((feed) => parseFloat(feed[`field${fieldNumber}`]));
                
                // Filter out invalid or NaN values
                var validFieldValues = fieldValues.filter(value => !isNaN(value));
                var rataRata = validFieldValues.reduce((total, value) => total + value, 0) / validFieldValues.length;

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
                    rataRataElement.textContent = `Rata-rata ${tipeData}: ${rataRata.toFixed(2)}`;
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
hitungRerata(2281820, 1, 60, 'suhu1');
hitungRerata(2281820, 2, 60, 'kelembaban1');
hitungRerata(2281820, 3, 60, 'tekanan1');
hitungRerata(2289583, 1, 60, 'suhu2');
hitungRerata(2289583, 2, 60, 'kelembaban2');
hitungRerata(2289583, 3, 60, 'tekanan2');
hitungRerata(2326256, 1, 60, 'suhu3');
hitungRerata(2326256, 2, 60, 'kelembaban3');
hitungRerata(2326256, 3, 60, 'tekanan3');