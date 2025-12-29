// HistoryPage.js
import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  FlatList,
  SafeAreaView,
  StatusBar,
} from 'react-native';
import { Calendar } from 'react-native-calendars';

const HistoryPage = () => {
  const [selectedDate, setSelectedDate] = useState('2024-11-24');
  const [selectedWorkoutType, setSelectedWorkoutType] = useState('all');

  // Mock data for workout history
  const workoutHistory = [
    {
      id: '1',
      date: 'November 24',
      time: '10:00',
      workoutName: 'Military Press',
      tags: ['arms', 'west'],
      exercises: [
        { name: 'Bicep Curl', sets: 3, reps: 8, weight: '50 lbs' },
        { name: 'Lat Raises', sets: 4, reps: 6, weight: '10 lbs' },
      ],
      notes: 'Pushing form',
    },
    {
      id: '2',
      date: 'November 23',
      time: '16:30',
      workoutName: 'Chest Day',
      tags: ['chest', 'core'],
      exercises: [
        { name: 'Bench Press', sets: 4, reps: 10, weight: '185 lbs' },
        { name: 'Incline Press', sets: 3, reps: 12, weight: '135 lbs' },
      ],
      notes: 'Felt strong today',
    },
    {
      id: '3',
      date: 'November 22',
      time: '09:15',
      workoutName: 'Back & Core',
      tags: ['back', 'core'],
      exercises: [
        { name: 'Pull-ups', sets: 4, reps: 10, weight: 'Body weight' },
        { name: 'Deadlifts', sets: 3, reps: 6, weight: '225 lbs' },
      ],
      notes: 'Focus on form',
    },
  ];

  // Training insights data
  const trainingInsights = [
    { id: '1', exercise: 'Bicep Curl', insight: 'Weight up 10 lbs!', type: 'improvement' },
    { id: '2', exercise: 'Bench Press', insight: 'Consistent progress over 4 weeks', type: 'trend' },
    { id: '3', exercise: 'Squats', insight: 'Form improved this week', type: 'form' },
  ];

  // Workout type filter buttons
  const workoutTypes = [
    { id: 'all', label: 'All' },
    { id: 'full-body', label: 'Full-body' },
    { id: 'core', label: 'Core' },
    { id: 'arms', label: 'Arms' },
    { id: 'back', label: 'Back' },
    { id: 'chest', label: 'Chest' },
  ];

  // Marked dates on calendar
  const markedDates = {
    '2024-11-24': { selected: true, selectedColor: '#4A90E2', marked: true },
    '2024-11-23': { marked: true, dotColor: '#4A90E2' },
    '2024-11-22': { marked: true, dotColor: '#4A90E2' },
    '2024-11-20': { marked: true, dotColor: '#4A90E2' },
    '2024-11-18': { marked: true, dotColor: '#4A90E2' },
  };

  const renderWorkoutItem = ({ item }) => (
    <View style={styles.workoutCard}>
      <View style={styles.workoutHeader}>
        <View>
          <Text style={styles.workoutTime}>{item.time}</Text>
          <Text style={styles.workoutDate}>{item.date}</Text>
        </View>
        <View>
          <Text style={styles.workoutName}>{item.workoutName}</Text>
          <View style={styles.tagsContainer}>
            {item.tags.map((tag, index) => (
              <View key={index} style={styles.tag}>
                <Text style={styles.tagText}>{tag}</Text>
              </View>
            ))}
          </View>
        </View>
      </View>

      <View style={styles.exercisesContainer}>
        {item.exercises.map((exercise, index) => (
          <View key={index} style={styles.exerciseRow}>
            <Text style={styles.exerciseName}>{exercise.name}</Text>
            <View style={styles.exerciseDetails}>
              <Text style={styles.exerciseDetail}>
                {exercise.sets} × {exercise.reps}
              </Text>
              <Text style={styles.exerciseWeight}>{exercise.weight}</Text>
            </View>
          </View>
        ))}
      </View>

      {item.notes && (
        <View style={styles.notesContainer}>
          <Text style={styles.notesText}>{item.notes}</Text>
        </View>
      )}
    </View>
  );

  const renderInsightItem = ({ item }) => (
    <View style={styles.insightCard}>
      <Text style={styles.insightExercise}>{item.exercise}</Text>
      <Text style={styles.insightText}>{item.insight}</Text>
    </View>
  );

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="dark-content" />
      
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.headerTitle}>History</Text>
      </View>

      <ScrollView showsVerticalScrollIndicator={false}>
        {/* Calendar Section */}
        <View style={styles.calendarSection}>
          <Text style={styles.sectionTitle}>Calendar</Text>
          <Calendar
            current={'2024-11-24'}
            minDate={'2024-11-01'}
            maxDate={'2024-11-30'}
            onDayPress={(day) => setSelectedDate(day.dateString)}
            markedDates={{
              ...markedDates,
              [selectedDate]: {
                ...markedDates[selectedDate],
                selected: true,
                selectedColor: '#4A90E2',
              },
            }}
            theme={{
              selectedDayBackgroundColor: '#4A90E2',
              todayTextColor: '#4A90E2',
              arrowColor: '#4A90E2',
              monthTextColor: '#333',
              textMonthFontWeight: '600',
              textDayFontSize: 16,
              textMonthFontSize: 16,
            }}
            style={styles.calendar}
          />
        </View>

        {/* Workout Type Filter */}
        <View style={styles.filterSection}>
          <ScrollView horizontal showsHorizontalScrollIndicator={false}>
            {workoutTypes.map((type) => (
              <TouchableOpacity
                key={type.id}
                style={[
                  styles.filterButton,
                  selectedWorkoutType === type.id && styles.filterButtonActive,
                ]}
                onPress={() => setSelectedWorkoutType(type.id)}
              >
                <Text
                  style={[
                    styles.filterButtonText,
                    selectedWorkoutType === type.id && styles.filterButtonTextActive,
                  ]}
                >
                  {type.label}
                </Text>
              </TouchableOpacity>
            ))}
          </ScrollView>
        </View>

        {/* Workout History List */}
        <View style={styles.historySection}>
          <View style={styles.sectionHeader}>
            <Text style={styles.sectionTitle}>Workout History</Text>
            <Text style={styles.seeAllText}>See All</Text>
          </View>
          <FlatList
            data={workoutHistory}
            renderItem={renderWorkoutItem}
            keyExtractor={(item) => item.id}
            scrollEnabled={false}
            ItemSeparatorComponent={() => <View style={styles.separator} />}
          />
        </View>

        {/* Training Insights */}
        <View style={styles.insightsSection}>
          <View style={styles.sectionHeader}>
            <Text style={styles.sectionTitle}>Training Insights</Text>
            <Text style={styles.seeAllText}>See All</Text>
          </View>
          <FlatList
            data={trainingInsights}
            renderItem={renderInsightItem}
            keyExtractor={(item) => item.id}
            horizontal
            showsHorizontalScrollIndicator={false}
            contentContainerStyle={styles.insightsList}
          />
        </View>

        {/* Achievements Preview */}
        <View style={styles.achievementsSection}>
          <Text style={styles.sectionTitle}>Recent Achievements</Text>
          <View style={styles.achievementRow}>
            <View style={styles.achievementCard}>
              <Text style={styles.achievementValue}>100 lbs</Text>
              <Text style={styles.achievementLabel}>Bench Press</Text>
            </View>
            <View style={styles.achievementCard}>
              <Text style={styles.achievementValue}>80 lbs</Text>
              <Text style={styles.achievementLabel}>Bicep Curl</Text>
            </View>
          </View>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  header: {
    paddingHorizontal: 16,
    paddingVertical: 20,
    backgroundColor: '#FFFFFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  headerTitle: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#333',
  },
  calendarSection: {
    backgroundColor: '#FFFFFF',
    marginTop: 8,
    padding: 16,
    borderRadius: 12,
    marginHorizontal: 16,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
    elevation: 2,
  },
  filterSection: {
    marginTop: 16,
    marginHorizontal: 16,
  },
  filterButton: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    backgroundColor: '#FFFFFF',
    borderRadius: 20,
    marginRight: 8,
    borderWidth: 1,
    borderColor: '#E0E0E0',
  },
  filterButtonActive: {
    backgroundColor: '#4A90E2',
    borderColor: '#4A90E2',
  },
  filterButtonText: {
    color: '#666',
    fontSize: 14,
    fontWeight: '500',
  },
  filterButtonTextActive: {
    color: '#FFFFFF',
  },
  historySection: {
    marginTop: 16,
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    marginHorizontal: 16,
    padding: 16,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
    elevation: 2,
  },
  sectionHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  sectionTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#333',
  },
  seeAllText: {
    color: '#4A90E2',
    fontSize: 14,
    fontWeight: '500',
  },
  workoutCard: {
    backgroundColor: '#F8F9FA',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  workoutHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
    marginBottom: 12,
  },
  workoutTime: {
    fontSize: 14,
    fontWeight: '600',
    color: '#333',
  },
  workoutDate: {
    fontSize: 12,
    color: '#666',
    marginTop: 2,
  },
  workoutName: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    textAlign: 'right',
  },
  tagsContainer: {
    flexDirection: 'row',
    justifyContent: 'flex-end',
    marginTop: 4,
  },
  tag: {
    backgroundColor: '#E3F2FD',
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 12,
    marginLeft: 4,
  },
  tagText: {
    fontSize: 10,
    color: '#1976D2',
    fontWeight: '500',
  },
  exercisesContainer: {
    marginTop: 8,
  },
  exerciseRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#E8E8E8',
  },
  exerciseName: {
    fontSize: 14,
    color: '#333',
    fontWeight: '500',
  },
  exerciseDetails: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  exerciseDetail: {
    fontSize: 14,
    color: '#666',
    marginRight: 12,
  },
  exerciseWeight: {
    fontSize: 14,
    fontWeight: '600',
    color: '#4A90E2',
    minWidth: 60,
    textAlign: 'right',
  },
  notesContainer: {
    marginTop: 12,
    padding: 8,
    backgroundColor: '#FFF3E0',
    borderRadius: 6,
  },
  notesText: {
    fontSize: 12,
    color: '#E65100',
    fontStyle: 'italic',
  },
  separator: {
    height: 12,
  },
  insightsSection: {
    marginTop: 16,
    marginHorizontal: 16,
  },
  insightsList: {
    paddingVertical: 8,
  },
  insightCard: {
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    padding: 16,
    marginRight: 12,
    width: 200,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
    elevation: 2,
  },
  insightExercise: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 4,
  },
  insightText: {
    fontSize: 14,
    color: '#4CAF50',
    fontWeight: '500',
  },
  achievementsSection: {
    marginTop: 16,
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    marginHorizontal: 16,
    padding: 16,
    marginBottom: 32,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
    elevation: 2,
  },
  achievementRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 12,
  },
  achievementCard: {
    alignItems: 'center',
    backgroundColor: '#F3F8FF',
    padding: 20,
    borderRadius: 12,
    minWidth: 120,
  },
  achievementValue: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#4A90E2',
  },
  achievementLabel: {
    fontSize: 14,
    color: '#666',
    marginTop: 4,
  },
  calendar: {
    borderRadius: 10,
    overflow: 'hidden',
  },
});

export default HistoryPage;
