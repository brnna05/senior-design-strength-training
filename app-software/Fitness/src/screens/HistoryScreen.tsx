import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import {Calendar} from 'react-native-calendars';
// https://www.npmjs.com/package/react-native-calendars
import InsightBox from '../components/InsightBox';

const HistoryScreen = () => {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>History</Text>
      <ScrollView style={styles.content}>
        <Calendar
          onDayPress={day => {
            console.log('selected day', day);
          }}
        />
        <Text style={styles.heading}>Insights</Text>

        <InsightBox
          type="success"
          message="5lbs up on your bicep curl max from last week! Great progress."
        />
        <InsightBox
          type="warning"
          message="Muscle strain detected. Remember to stretch before and after your workouts."
        />
        <InsightBox
          type="error"
          message="Incorrect form detected during your bench press session."
        />

        <Text style={styles.heading}>Achievements</Text>

      </ScrollView>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: 'white',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    padding: 20,
    paddingTop: 65,
    backgroundColor: 'white',
  },
  heading: {
    fontSize: 20,
    fontWeight: '600',
    padding: 20
  },
  content: {
    flex: 1,
    padding: 10
    ,
  },
  placeholder: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
    marginTop: 50,
  },
  insightsBox: {
  }
});

export default HistoryScreen;