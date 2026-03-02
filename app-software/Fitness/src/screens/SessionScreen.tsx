import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Alert,
  Modal,
} from 'react-native';

type SessionPhase = 'idle' | 'working' | 'break' | 'done';

const TOTAL_SETS = 3;
const TOTAL_REPS = 10;
const BREAK_DURATION_SEC = 60;

const SessionScreen = () => {
  const [phase, setPhase] = useState<SessionPhase>('idle');
  const [currentSet, setCurrentSet] = useState(1);
  const [currentRep, setCurrentRep] = useState(1);
  const [completedSets, setCompletedSets] = useState<number[]>([]);
  const [breakSecondsLeft, setBreakSecondsLeft] = useState(BREAK_DURATION_SEC);
  const [breakTimer, setBreakTimer] = useState<ReturnType<typeof setInterval> | null>(null);
  const [summaryVisible, setSummaryVisible] = useState(false);

  const startWorkout = () => {
    setPhase('working');
    setCurrentSet(1);
    setCurrentRep(1);
    setCompletedSets([]);
  };

  const completeRep = () => {
    if (currentRep < TOTAL_REPS) {
      setCurrentRep(r => r + 1);
    } else {
      // last rep of the set completed
      const newCompleted = [...completedSets, currentSet];
      setCompletedSets(newCompleted);

      // prompt break if not last set
      if (currentSet < TOTAL_SETS) {
        setPhase('break');
        startBreakTimer();
        setCurrentRep(1);
        setCurrentSet(s => s + 1);
      } else {
        endWorkout(newCompleted);
      }
    }
  };

  const startBreakTimer = () => {
    setBreakSecondsLeft(BREAK_DURATION_SEC);
    const id = setInterval(() => {
      setBreakSecondsLeft(prev => {
        if (prev <= 1) {
          clearInterval(id);
          return 0;
        }
        return prev - 1;
      });
    }, 1000);
    setBreakTimer(id);
  };

  const returnFromBreak = () => {
    if (breakTimer) clearInterval(breakTimer);
    setBreakTimer(null);
    setPhase('working');
  };

  const endWorkout = (finalSets?: number[]) => {
    if (breakTimer) clearInterval(breakTimer);
    setBreakTimer(null);
    setPhase('done');
    setSummaryVisible(true);
  };

  const handleEndPress = () => {
    Alert.alert(
      'End Workout',
      'Are you sure you want to end this workout?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'End', style: 'destructive', onPress: () => endWorkout() },
      ],
    );
  };

  const resetSession = () => {
    setSummaryVisible(false);
    setPhase('idle');
    setCurrentSet(1);
    setCurrentRep(1);
    setCompletedSets([]);
  };

  const setsCompleted = completedSets.length;
  const progressPercent =
    phase === 'working' || phase === 'break'
      ? ((setsCompleted * TOTAL_REPS + (currentRep - 1)) /
          (TOTAL_SETS * TOTAL_REPS)) *
        100
      : 0;


  if (phase === 'idle') {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>Live Session</Text>
        <View style={styles.centerContent}>
          <Text style={styles.exerciseLabel}>Bicep Curl</Text>
          <Text style={styles.subLabel}>
            {TOTAL_SETS} sets × {TOTAL_REPS} reps
          </Text>
          <TouchableOpacity style={styles.startButton} onPress={startWorkout}>
            <Text style={styles.startButtonText}>Start Workout</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  // break screen
  if (phase === 'break') {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>Live Session</Text>

        <View style={styles.progressBarBg}>
          <View style={[styles.progressBarFill, { width: `${progressPercent}%` }]} />
        </View>
        <Text style={styles.progressText}>
          {setsCompleted} / {TOTAL_SETS} sets complete
        </Text>

        <View style={styles.centerContent}>
          <Text style={styles.breakTitle}>Break Time</Text>
          <Text style={styles.breakTimer}>{breakSecondsLeft}s</Text>
          <Text style={styles.breakSub}>
            Up next — Set {currentSet} of {TOTAL_SETS}
          </Text>

          <TouchableOpacity style={styles.primaryButton} onPress={returnFromBreak}>
            <Text style={styles.primaryButtonText}>Return from Break</Text>
          </TouchableOpacity>

          <TouchableOpacity style={styles.endButton} onPress={handleEndPress}>
            <Text style={styles.endButtonText}>End Workout</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  /* TODO: 
   * - change to update based on wearable device data instead of button presses 
   * - add warnings if form is incorrect or if user is struggling to complete reps
  */
  // wourkout in progress
  if (phase === 'working') {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>Live Session</Text>

        <View style={styles.progressBarBg}>
          <View style={[styles.progressBarFill, { width: `${progressPercent}%` }]} />
        </View>
        <Text style={styles.progressText}>
          {setsCompleted} / {TOTAL_SETS} sets complete
        </Text>

        <View style={styles.centerContent}>
          <Text style={styles.exerciseLabel}>Bicep Curl</Text>

          <View style={styles.statRow}>
            <View style={styles.statBox}>
              <Text style={styles.statValue}>{currentSet}</Text>
              <Text style={styles.statLabel}>SET</Text>
            </View>
            <View style={styles.divider} />
            <View style={styles.statBox}>
              <Text style={styles.statValue}>{currentRep}</Text>
              <Text style={styles.statLabel}>REP</Text>
            </View>
            <View style={styles.divider} />
            <View style={styles.statBox}>
              <Text style={styles.statValue}>{TOTAL_REPS}</Text>
              <Text style={styles.statLabel}>GOAL</Text>
            </View>
          </View>

          {/* Rep dots */}
          <View style={styles.repDots}>
            {Array.from({ length: TOTAL_REPS }).map((_, i) => (
              <View
                key={i}
                style={[
                  styles.dot,
                  i < currentRep - 1 && styles.dotDone,
                  i === currentRep - 1 && styles.dotCurrent,
                ]}
              />
            ))}
          </View>

          <TouchableOpacity style={styles.primaryButton} onPress={completeRep}>
            <Text style={styles.primaryButtonText}>
              {currentRep < TOTAL_REPS ? `Complete Rep ${currentRep}` : 'Finish Set'}
            </Text>
          </TouchableOpacity>

          <TouchableOpacity style={styles.breakButtonAlt} onPress={() => {
            setPhase('break');
            startBreakTimer();
          }}>
            <Text style={styles.breakButtonAltText}>Take a Break</Text>
          </TouchableOpacity>

          <TouchableOpacity style={styles.endButton} onPress={handleEndPress}>
            <Text style={styles.endButtonText}>End Workout</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Live Session</Text>
      <Modal visible={summaryVisible} transparent animationType="slide">
        <View style={styles.modalOverlay}>
          <View style={styles.modalCard}>
            <Text style={styles.modalTitle}>Workout Summary</Text>
            <Text style={styles.summaryLine}>Exercise: Bicep Curl</Text>
            <Text style={styles.summaryLine}>
              Sets Completed: {completedSets.length} / {TOTAL_SETS}
            </Text>
            <Text style={styles.summaryLine}>Reps per Set: {TOTAL_REPS}</Text>
            <TouchableOpacity
              style={styles.primaryButton}
              onPress={resetSession}>
              <Text style={styles.primaryButtonText}>Done</Text>
            </TouchableOpacity>
          </View>
        </View>
      </Modal>
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
  progressBarBg: {
    height: 6,
    backgroundColor: '#E0E0E0',
    marginHorizontal: 20,
    borderRadius: 3,
    marginTop: 4,
  },
  progressBarFill: {
    height: 6,
    backgroundColor: '#658e58',
    borderRadius: 3,
  },
  progressText: {
    fontSize: 12,
    color: '#888',
    textAlign: 'right',
    marginRight: 20,
    marginTop: 4,
  },
  centerContent: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingHorizontal: 30,
  },
  exerciseLabel: { 
    fontSize: 28, 
    fontWeight: '700', 
    marginBottom: 6 
  },
  subLabel: { 
    fontSize: 15, 
    color: '#888', 
    marginBottom: 36 
  },

  statRow: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#F7F7F7',
    borderRadius: 16,
    paddingVertical: 20,
    paddingHorizontal: 30,
    marginBottom: 24,
    width: '100%',
    justifyContent: 'space-around',
  },
  statBox: { 
    alignItems: 'center', 
    flex: 1 
  },
  statValue: { 
    fontSize: 42, 
    fontWeight: '800', 
    color: '#1A1A1A' 
  },
  statLabel: { 
    fontSize: 11, 
    fontWeight: '600', 
    color: '#999', 
    letterSpacing: 1.2, 
    marginTop: 2 
  },
  divider: { 
    width: 1, 
    height: 50, 
    backgroundColor: '#DDD' 
  },

  // rep dots
  repDots: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'center',
    marginBottom: 28,
    gap: 6,
  },
  dot: {
    width: 14,
    height: 14,
    borderRadius: 7,
    backgroundColor: '#E0E0E0',
    margin: 3,
  },
  dotDone: { 
    backgroundColor: '#658e58' 
  },
  dotCurrent: { 
    backgroundColor: '#1A1A1A' 
  },

  // buttons
  primaryButton: {
    backgroundColor: '#D6E0D3',
    paddingHorizontal: 40,
    paddingVertical: 16,
    borderRadius: 30,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#636363',
    width: '100%',
    alignItems: 'center',
  },
  primaryButtonText: { 
    color: 'black', 
    fontSize: 16, 
    fontWeight: '600' 
  },

  // break button during workout
  breakButtonAlt: {
    backgroundColor: '#F1E5C4',
    paddingHorizontal: 40,
    paddingVertical: 16,
    borderRadius: 30,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#636363',
    width: '100%',
    alignItems: 'center',
  },
  breakButtonAltText: { 
    color: 'black', 
    fontSize: 16, 
    fontWeight: '600' 
  },

  // end button
  endButton: {
    paddingHorizontal: 40,
    paddingVertical: 14,
    borderRadius: 30,
    borderWidth: 1,
    borderColor: '#636363',
    width: '100%',
    alignItems: 'center',
    backgroundColor: '#DCBDBD',
  },
  endButtonText: { 
    color: '#000000', 
    fontSize: 16, 
    fontWeight: '600' 
  },

  // start button
  startButton: {
    backgroundColor: '#D6E0D3',
    paddingHorizontal: 40,
    paddingVertical: 15,
    borderRadius: 30,
    marginTop: 20,
    borderWidth: 1,
    borderColor: '#636363',
  },
  startButtonText: { 
    color: 'black', 
    fontSize: 18, 
    fontWeight: '500' 
  },

  // break screen
  breakTitle: { 
    fontSize: 32,
    fontWeight: '700',
    marginBottom: 10 
  },
  breakTimer: {
    fontSize: 64, 
    fontWeight: '800', 
    color: '#658e58', 
    marginBottom: 6 
  },
  breakSub: { 
    fontSize: 15, 
    color: '#888', 
    marginBottom: 36 
  },

  // summary
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.4)',
    justifyContent: 'flex-end',
  },
  modalCard: {
    backgroundColor: 'white',
    borderTopLeftRadius: 24,
    borderTopRightRadius: 24,
    padding: 30,
    paddingBottom: 50,
  },
  modalTitle: { 
    fontSize: 22, 
    fontWeight: '700', 
    marginBottom: 20 },
  summaryLine: { 
    fontSize: 16, 
    color: '#444', 
    marginBottom: 10 
  },
});

export default SessionScreen;