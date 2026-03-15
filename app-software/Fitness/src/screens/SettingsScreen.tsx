import React from 'react';
import { View, Text, StyleSheet, ScrollView, Button, TouchableOpacity } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { RootStackParamList } from '../navigation/RootNavigator';

type SettingsMode = 'idle' | 'profile' | 'data';

const SettingsScreen = () => {
  const navigation = useNavigation<NativeStackNavigationProp<RootStackParamList>>();

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <ScrollView style={styles.content}>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Account</Text>
          <Text style={styles.settingItem}>Profile</Text>
          <Text style={styles.settingItem}>Data Management</Text>
          <TouchableOpacity 
            style={styles.logoutButton}
            onPress={() => {
              navigation.replace('Login');
            }}
          >
            <Text style={styles.settingItem}>Log Out</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Bluetooth</Text>
          <View style={styles.settingItem}>
            <Text style={styles.settingLabel}>Connect Device</Text>
            <Text style={styles.settingValue}>Not Connected</Text>
          </View>
        </View>
      </ScrollView>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    padding: 20,
    paddingTop: 65,
    backgroundColor: 'white',
  },
  content: {
    flex: 1,
  },
  section: {
    backgroundColor: 'white',
    marginTop: 20,
    paddingVertical: 10,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#666',
    paddingHorizontal: 20,
    paddingVertical: 10,
  },
  settingItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 20,
    paddingVertical: 15,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  settingLabel: {
    fontSize: 16,
  },
  textContainer: {
  flexDirection: 'column',
  },
  settingDescription: {
    fontSize: 12,
    color: '#666',
    margin: 6,
  },
  settingValue: {
    fontSize: 14,
    color: '#666',
  },
  logoutButton: {
    backgroundColor: 'white',
    borderRadius: 8,
    width: '100%',
  },
});

export default SettingsScreen;