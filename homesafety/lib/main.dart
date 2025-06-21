import 'dart:io';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/material.dart';
import 'package:homesafety/homepage.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'package:intl/date_symbol_data_local.dart';

Future<void> _firebaseMessagingBackgroundHandler(RemoteMessage message) async {
  // Arka planda alınan bildirimler burada işlenir.
  print('Background message received: ${message.messageId}');
}

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  Platform.isAndroid ?
    await Firebase.initializeApp(
      options: const FirebaseOptions(
        apiKey: 'AIzaSyCfqh1LYoPTy6xekYzRpjx-KCnkQ-1Y7AE',
        appId: '1:329039616607:android:864a950f31875fd7cc19ea',
        messagingSenderId: '329039616607',
        projectId: 'home-8262d',
      ),
    )
   
    : await Firebase.initializeApp();
  
  await initializeDateFormatting('tr_TR', null);
  FirebaseMessaging.onBackgroundMessage(_firebaseMessagingBackgroundHandler);
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Flutter Demo',
      theme: ThemeData(

        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: HomePage(),
    );
  }
}

