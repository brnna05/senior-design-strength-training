import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { Calendar } from 'react-native-calendars';
import { useWorkouts, CompletedWorkout } from '../context/WorkoutContext';
// import InsightBox from '../components/InsightBox';

const HistoryScreen = () => {
  const { workouts } = useWorkouts();
  const [selectedDate, setSelectedDate] = useState<string | null>(null);

  // Build marked dates from saved workouts
  const markedDates: Record<string, { marked: boolean; dotColor: string; selected?: boolean; selectedColor?: string }> = {};
  workouts.forEach(w => {
    markedDates[w.date] = { marked: true, dotColor: '#658e58' };
  });
  if (selectedDate) {
    markedDates[selectedDate] = {
      ...(markedDates[selectedDate] ?? { marked: false, dotColor: '#658e58' }),
      selected: true,
      selectedColor: '#658e58',
    };
  }

  // Workouts on the selected date
  const dayWorkouts = selectedDate
    ? workouts.filter(w => w.date === selectedDate)
    : [];

  return (
    <View style={styles.container}>
      <Text style={styles.title}>History</Text>
      <ScrollView style={styles.content}>
        <Calendar
          markedDates={markedDates}
          onDayPress={day => setSelectedDate(
            selectedDate === day.dateString ? null : day.dateString
          )}
          theme={{
            todayTextColor: '#658e58',
            selectedDayBackgroundColor: '#658e58',
            arrowColor: '#658e58',
            dotColor: '#658e58',
          }}
        />

        {/* Selected date workouts */}
        {selectedDate && (
          <View style={styles.daySection}>
            <Text style={styles.dayTitle}>{formatDate(selectedDate)}</Text>

            {dayWorkouts.length === 0 ? (
              <Text style={styles.emptyDay}>No workout recorded on this day.</Text>
            ) : (
              dayWorkouts.map(workout => (
                <WorkoutCard key={workout.id} workout={workout} />
              ))
            )}
          </View>
        )}

        {/* Insights */}
        {/* <Text style={styles.heading}>Insights</Text>
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
        /> */}

      </ScrollView>
    </View>
  );
};


const WorkoutCard = ({ workout }: { workout: CompletedWorkout }) => (
  <View style={cardStyles.container}>
    <View style={cardStyles.card}>
      <View style={cardStyles.header}>
        <Text style={cardStyles.exercise}>{workout.exercise}</Text>
        <Text style={cardStyles.exercise}>{workout.side} Arm</Text>
      </View>

      <View style={cardStyles.statsRow}>
        <View style={cardStyles.stat}>
          <Text style={cardStyles.statValue}>{workout.sets.length}</Text>
          <Text style={cardStyles.statLabel}>SETS</Text>
        </View>
        <View style={cardStyles.stat}>
          <Text style={cardStyles.statValue}>{workout.totalReps}</Text>
          <Text style={cardStyles.statLabel}>TOTAL REPS</Text>
        </View>
      </View>

      <View style={cardStyles.setList}>
        {workout.sets.map(s => (
          <View key={s.setNumber} style={cardStyles.setRow}>
            <Text style={cardStyles.setLabel}>Set {s.setNumber}</Text>
            <View style={cardStyles.repBar}>
              <View
                style={[
                  cardStyles.repBarFill,
                  { width: `${(s.repsCompleted / Math.max(...workout.sets.map(x => x.repsCompleted))) * 100}%` },
                ]}
              />
            </View>
            <Text style={cardStyles.setReps}>{s.repsCompleted} reps</Text>
          </View>
        ))}
      </View>
    </View>
  </View>
);

const formatDate = (dateStr: string) => {
  const [year, month, day] = dateStr.split('-').map(Number);
  const date = new Date(year, month - 1, day);
  return date.toLocaleDateString('en-US', { weekday: 'short', month: 'short', day: 'numeric', year: 'numeric' });
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: 'white'},
  containerCard: { flex: 1, alignItems: 'center', backgroundColor: 'white' },
  title: { fontSize: 24, fontWeight: 'bold', padding: 20, paddingTop: 65, backgroundColor: 'white' },
  content: { flex: 1, padding: 10 },
  heading: { fontSize: 20, fontWeight: '600', padding: 10, marginTop: 6 },
  daySection: { marginTop: 10, marginBottom: 4 },
  dayTitle: { fontSize: 17, fontWeight: '700', paddingHorizontal: 10, paddingBottom: 6, color: '#1A1A1A' },
  emptyDay: { fontSize: 14, color: '#999', paddingHorizontal: 10, paddingBottom: 10 },
  emptyText: { fontSize: 15, color: '#999', textAlign: 'center', marginTop: 16, marginBottom: 16 },
});

const cardStyles = StyleSheet.create({
  container: { flex: 1, alignItems: 'center', backgroundColor: 'white' },
  card: {
    backgroundColor: '#FAFAFA',
    borderRadius: 12,
    borderWidth: 1,
    borderColor: '#E0E0E0',
    marginHorizontal: 10,
    marginBottom: 12,
    padding: 16,
    width: '90%',
  },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  exercise: { fontSize: 16, fontWeight: '700', color: '#1A1A1A' },
  date: { fontSize: 12, color: '#888' },
  statsRow: { flexDirection: 'row', marginBottom: 14 },
  stat: { flex: 1, alignItems: 'center' },
  statValue: { fontSize: 22, fontWeight: '800', color: '#1A1A1A' },
  statLabel: { fontSize: 10, color: '#999', fontWeight: '600', letterSpacing: 1, marginTop: 2 },
  setList: { gap: 8 },
  setRow: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  setLabel: { fontSize: 12, color: '#666', width: 36 },
  repBar: { flex: 1, height: 6, backgroundColor: '#E0E0E0', borderRadius: 3 },
  repBarFill: { height: 6, backgroundColor: '#658e58', borderRadius: 3 },
  setReps: { fontSize: 12, color: '#444', width: 48, textAlign: 'right' },
});

export default HistoryScreen;