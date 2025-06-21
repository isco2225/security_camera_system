const functions = require('firebase-functions');
const admin = require('firebase-admin');
admin.initializeApp();

const firestore = admin.firestore();
const realtimeDB = admin.database();

exports.syncRealtimeToFirestore = functions.database
  .ref('/url_and_uploadedDate/{fileId}')
  .onUpdate(async (change, context) => {
    const beforeData = change.before.val(); // Güncelleme öncesi veriler
    const afterData = change.after.val();  // Güncelleme sonrası veriler

    // Veriler değişmiş mi kontrol et
    if (
      (beforeData.url !== afterData.url) 
    ) {
      try {
        // Firestore'a veri ekle
        await firestore.collection('images').add({
          url: afterData.url,
          uploadedDate: afterData.uploadedDate,
          timestamp: admin.firestore.FieldValue.serverTimestamp(),
        });

        console.log('Data synced to Firestore successfully');

        // Bildirim Gönder
        const message = {
          notification: {
            title: 'Kapınızda biri var!',
            body: `Kim olduğunu görmek için tıkla.`,
          },
          topic: 'images', // "images" topic'ine abone olan cihazlara gönderilecek
        };

        await admin.messaging().send(message);
        console.log('Bildirim başarıyla gönderildi');
      } catch (error) {
        console.error('Error syncing data to Firestore or sending notification:', error);
      }
    }
  });
