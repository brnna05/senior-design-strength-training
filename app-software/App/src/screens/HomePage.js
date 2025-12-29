// HomeScreen.js
import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  SafeAreaView,
  Image,
  StatusBar
} from 'react-native';
import { exercises, getExercisesByPrimaryMuscles } from './exercises';

const HomeScreen = ({ navigation }) => {
  // Sample user data - you can replace this with actual user data
  const userData = {
    name: "Brianna",
    streak: 7,
    nextWorkout: "Push Day",
    lastWorkout: "Pull Day"
  };

  // Quick stats
  const quickStats = [
    { label: 'Workouts', value: '12' },
    { label: 'PRs', value: '5' },
    { label: 'Streak', value: `${userData.streak} days` },
  ];

  // Popular muscle groups for quick access
  const popularMuscleGroups = [
    { name: 'Chest', muscle: 'pectorals', icon: '💪', color: '#FF6B6B' },
    { name: 'Back', muscle: 'lats', icon: '🏋️', color: '#4ECDC4' },
    { name: 'Legs', muscle: 'quadriceps', icon: '🦵', color: '#45B7D1' },
    { name: 'Arms', muscle: 'biceps', icon: '💪', color: '#96CEB4' },
    { name: 'Core', muscle: 'abdominals', icon: '🎯', color: '#FFEAA7' },
    { name: 'Shoulders', muscle: 'deltoids', icon: '👤', color: '#DDA0DD' },
  ];

  const handleMuscleGroupPress = (muscle) => {
    const exercisesForMuscle = getExercisesByPrimaryMuscles(muscle);
    navigation.navigate('ExerciseList', { 
      exercises: exercisesForMuscle,
      title: `${muscle} Exercises`
    });
  };

  const handleQuickStart = () => {
    navigation.navigate('QuickWorkout');
  };

  const handleViewProgress = () => {
    navigation.navigate('Progress');
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="dark-content" />
      <ScrollView showsVerticalScrollIndicator={false}>
        {/* Header */}
        <View style={styles.header}>
          <View>
            <Text style={styles.greeting}>Hello, {userData.name}! 👋</Text>
            <Text style={styles.subtitle}>Ready for your next workout?</Text>
          </View>
          <TouchableOpacity style={styles.profileButton}>
            <Text style={styles.profileIcon}>👤</Text>
          </TouchableOpacity>
        </View>

        {/* Quick Stats */}
        <View style={styles.statsContainer}>
          {quickStats.map((stat, index) => (
            <View key={index} style={styles.statCard}>
              <Text style={styles.statValue}>{stat.value}</Text>
              <Text style={styles.statLabel}>{stat.label}</Text>
            </View>
          ))}
        </View>

        {/* Quick Actions */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Quick Actions</Text>
          <View style={styles.quickActions}>
            <TouchableOpacity 
              style={[styles.quickAction, styles.primaryAction]}
              onPress={handleQuickStart}
            >
              <Text style={styles.actionIcon}>⚡</Text>
              <Text style={styles.actionText}>Quick Start</Text>
              <Text style={styles.actionSubtext}>Begin workout now</Text>
            </TouchableOpacity>

            <TouchableOpacity 
              style={[styles.quickAction, styles.secondaryAction]}
              onPress={handleViewProgress}
            >
              <Text style={styles.actionIcon}>📊</Text>
              <Text style={styles.actionText}>View Progress</Text>
              <Text style={styles.actionSubtext}>See your gains</Text>
            </TouchableOpacity>
          </View>
        </View>

        {/* Recent Activity */}
        <View style={styles.section}>
          <View style={styles.sectionHeader}>
            <Text style={styles.sectionTitle}>Recent Activity</Text>
            <TouchableOpacity>
              <Text style={styles.seeAllText}>See All</Text>
            </TouchableOpacity>
          </View>
          <View style={styles.recentActivity}>
            <View style={styles.activityItem}>
              <View style={styles.activityIcon}>
                <Text>✅</Text>
              </View>
              <View style={styles.activityInfo}>
                <Text style={styles.activityTitle}>Completed {userData.lastWorkout}</Text>
                <Text style={styles.activityTime}>Yesterday, 6:30 PM</Text>
              </View>
            </View>
            <View style={styles.activityItem}>
              <View style={styles.activityIcon}>
                <Text>🎯</Text>
              </View>
              <View style={styles.activityInfo}>
                <Text style={styles.activityTitle}>New PR: Bench Press</Text>
                <Text style={styles.activityTime}>135 lbs → 145 lbs</Text>
              </View>
            </View>
          </View>
        </View>

        {/* Muscle Groups */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Muscle Groups</Text>
          <View style={styles.muscleGrid}>
            {popularMuscleGroups.map((group, index) => (
              <TouchableOpacity
                key={index}
                style={[styles.muscleCard, { backgroundColor: group.color }]}
                onPress={() => handleMuscleGroupPress(group.muscle)}
              >
                <Text style={styles.muscleIcon}>{group.icon}</Text>
                <Text style={styles.muscleName}>{group.name}</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        {/* Upcoming Workout */}
        <View style={styles.section}>
          <View style={styles.sectionHeader}>
            <Text style={styles.sectionTitle}>Next Workout</Text>
            <TouchableOpacity>
              <Text style={styles.seeAllText}>Edit</Text>
            </TouchableOpacity>
          </View>
          <TouchableOpacity style={styles.nextWorkoutCard}>
            <View style={styles.workoutInfo}>
              <Text style={styles.workoutName}>{userData.nextWorkout}</Text>
              <Text style={styles.workoutDetails}>6 exercises • 45 minutes</Text>
              <Text style={styles.workoutTime}>Scheduled for today</Text>
            </View>
            <View style={styles.startButton}>
              <Text style={styles.startButtonText}>START</Text>
            </View>
          </TouchableOpacity>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F8F9FA',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 20,
    paddingTop: 20,
    paddingBottom: 10,
  },
  greeting: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#1A1A1A',
  },
  subtitle: {
    fontSize: 16,
    color: '#666',
    marginTop: 4,
  },
  profileButton: {
    width: 44,
    height: 44,
    borderRadius: 22,
    backgroundColor: '#E9ECEF',
    justifyContent: 'center',
    alignItems: 'center',
  },
  profileIcon: {
    fontSize: 20,
  },
  statsContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingHorizontal: 20,
    marginVertical: 20,
  },
  statCard: {
    backgroundColor: 'white',
    padding: 16,
    borderRadius: 12,
    alignItems: 'center',
    flex: 1,
    marginHorizontal: 5,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 3,
    elevation: 3,
  },
  statValue: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#1A1A1A',
  },
  statLabel: {
    fontSize: 12,
    color: '#666',
    marginTop: 4,
  },
  section: {
    paddingHorizontal: 20,
    marginBottom: 24,
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
    color: '#1A1A1A',
  },
  seeAllText: {
    fontSize: 14,
    color: '#007AFF',
    fontWeight: '600',
  },
  quickActions: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  quickAction: {
    flex: 1,
    padding: 20,
    borderRadius: 16,
    marginHorizontal: 5,
  },
  primaryAction: {
    backgroundColor: '#007AFF',
  },
  secondaryAction: {
    backgroundColor: '#5856D6',
  },
  actionIcon: {
    fontSize: 24,
    marginBottom: 8,
  },
  actionText: {
    fontSize: 16,
    fontWeight: 'bold',
    color: 'white',
    marginBottom: 4,
  },
  actionSubtext: {
    fontSize: 12,
    color: 'rgba(255,255,255,0.8)',
  },
  recentActivity: {
    backgroundColor: 'white',
    borderRadius: 12,
    padding: 16,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 3,
    elevation: 3,
  },
  activityItem: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 12,
  },
  activityIcon: {
    width: 40,
    height: 40,
    borderRadius: 20,
    backgroundColor: '#F8F9FA',
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  activityInfo: {
    flex: 1,
  },
  activityTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#1A1A1A',
  },
  activityTime: {
    fontSize: 14,
    color: '#666',
    marginTop: 2,
  },
  muscleGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  muscleCard: {
    width: '48%',
    aspectRatio: 1,
    borderRadius: 16,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 12,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 3,
    elevation: 3,
  },
  muscleIcon: {
    fontSize: 32,
    marginBottom: 8,
  },
  muscleName: {
    fontSize: 16,
    fontWeight: '600',
    color: '#1A1A1A',
  },
  nextWorkoutCard: {
    backgroundColor: 'white',
    borderRadius: 16,
    padding: 20,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 3,
    elevation: 3,
  },
  workoutInfo: {
    flex: 1,
  },
  workoutName: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#1A1A1A',
    marginBottom: 4,
  },
  workoutDetails: {
    fontSize: 14,
    color: '#666',
    marginBottom: 2,
  },
  workoutTime: {
    fontSize: 14,
    color: '#007AFF',
    fontWeight: '600',
  },
  startButton: {
    backgroundColor: '#007AFF',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 20,
  },
  startButtonText: {
    color: 'white',
    fontWeight: 'bold',
    fontSize: 14,
  },
});

export default HomeScreen;