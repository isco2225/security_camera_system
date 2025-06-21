import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:intl/intl.dart'; 
import 'homepage.dart'; 

class Archive extends StatefulWidget {
  @override
  _ArchiveState createState() => _ArchiveState();
}

class _ArchiveState extends State<Archive> {
  int _selectedIndex = 1; 
  DateTime? selectedDate;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.greenAccent,
                flexibleSpace: SafeArea(
          child: Center(
            child: Text(
              "Arşiv", 
              style: TextStyle(fontSize: 20),
            ),
          ),
        ),
        actions: [
          IconButton(
            icon: Icon(Icons.calendar_today),
            onPressed: () async {
              DateTime? pickedDate = await showDatePicker(
                context: context,
                initialDate: DateTime.now(),
                firstDate: DateTime(2000),
                lastDate: DateTime.now(),
              );
              if (pickedDate != null) {
                setState(() {
                  selectedDate = pickedDate;
                });
              }
            },
          ),
           if (selectedDate != null) 
            IconButton(
              icon: Icon(Icons.clear), 
              onPressed: () {
                setState(() {
                  selectedDate = null; 
                });
              },
            ),
        ],
      ),
      body: FutureBuilder<QuerySnapshot>(
        future: getImages(),
        builder: (context, snapshot) {
          if (snapshot.connectionState == ConnectionState.waiting) {
            return Center(child: CircularProgressIndicator());
          }
          if (snapshot.hasError) {
            return Center(child: Text('Error fetching data'));
          }
          if (!snapshot.hasData || snapshot.data!.docs.isEmpty) {
            return Center(child: Text('Herhangi bir resim yok'));
          }

          final images = snapshot.data!.docs;

          return ListView.builder(
            itemCount: images.length,
            itemBuilder: (context, index) {
              final image = images[index];
              final imageUrl = image['url'];
              final Timestamp uploadedDate = image['timestamp'];

            
              final DateTime dateTime = uploadedDate.toDate();
              final String formattedDay = DateFormat('EEEE', 'tr_TR').format(dateTime); 
              final String formattedTime = DateFormat('HH:mm').format(dateTime); 

              return Container(
                margin: EdgeInsets.symmetric(vertical: 10, horizontal: 15),
                padding: EdgeInsets.all(10),
                decoration: BoxDecoration(
                  border: Border.all(color: Colors.grey),
                  borderRadius: BorderRadius.circular(10),
                  color: Colors.white,
                ),
                child: Column(
                  children: [
                    Image.network(
                      imageUrl,
                      width: double.infinity,
                      height: 200,
                      fit: BoxFit.cover,
                    ),
                    SizedBox(height: 10),
                    Text(
                      '$formattedDay, $formattedTime',
                      style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                    ),
                  ],
                ),
              );
            },
          );
        },
      ),
      bottomNavigationBar: BottomNavigationBar(
        items: const <BottomNavigationBarItem>[
          BottomNavigationBarItem(
            icon: Icon(Icons.home),
            label: 'Ana Sayfa',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.photo_library),
            label: 'Arşiv',
          ),
        ],
        currentIndex: _selectedIndex,
        selectedItemColor: Colors.blue,
        onTap: (index) {
          if (index == 0) {
            Navigator.pushReplacement(
              context,
              MaterialPageRoute(builder: (context) => HomePage()),
            );
          } else {
            setState(() {
              _selectedIndex = index;
            });
          }
        },
      ),
    );
  }

  Future<QuerySnapshot> getImages() {
    CollectionReference images = FirebaseFirestore.instance.collection('images');
    if (selectedDate != null) {
      
      String formattedDate = DateFormat('yyyy/MM/dd').format(selectedDate!);
      return images
          .where('uploadedDate', isGreaterThanOrEqualTo: formattedDate)
          .where('uploadedDate', isLessThan: '$formattedDate\T23:59:59.999Z')
          .orderBy('uploadedDate', descending: true)
          .get();
    } else {
      
      return images.orderBy('timestamp', descending: true).get();
    }
  }

  DateTime parseCustomDate(String dateString) {
  try {
  
    DateFormat customFormat = DateFormat("dd/MM/yyyy HH:mm");
    return customFormat.parse(dateString);
  } catch (e) {
    throw FormatException("Invalid date format: $dateString");
  }
}
}
