import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Alert,
  Modal,
} from 'react-native';
import { useWorkouts, CompletedSet } from '../context/WorkoutContext';

type SessionPhase = 'idle' | 'working' | 'break' | 'done';

const DEFAULT_SETS = 3;
const DEFAULT_REPS = 10;
const BREAK_DURATION_SEC = 60;

interface StepperProps {
  label: string;
  value: number;
  onDecrement: () => void;
  onIncrement: () => void;
  min?: number;
}

const Stepper: React.FC<StepperProps> = ({ label, value, onDecrement, onIncrement, min = 1 }) => (
  <View style={styles.stepperRow}>
    <Text style={styles.stepperLabel}>{label}</Text>
    <View style={styles.stepperControls}>
      <TouchableOpacity
        style={[styles.stepperBtn, value <= min && styles.stepperBtnDisabled]}
        onPress={onDecrement}
        disabled={value <= min}>
        <Text style={styles.stepperBtnText}>−</Text>
      </TouchableOpacity>
      <Text style={styles.stepperValue}>{value}</Text>
      <TouchableOpacity style={styles.stepperBtn} onPress={onIncrement}>
        <Text style={styles.stepperBtnText}>+</Text>
      </TouchableOpacity>
    </View>
  </View>
);

const SessionScreen = () => {
  const { saveWorkout } = useWorkouts();

  const [phase, setPhase] = useState<SessionPhase>('idle');
  const [totalSets, setTotalSets] = useState(DEFAULT_SETS);
  const [totalReps, setTotalReps] = useState(DEFAULT_REPS);
  const [currentSet, setCurrentSet] = useState(1);
  const [currentRep, setCurrentRep] = useState(1);
  const [completedSets, setCompletedSets] = useState<number[]>([]);
  const [setLog, setSetLog] = useState<CompletedSet[]>([]);
  const [summaryLog, setSummaryLog] = useState<CompletedSet[]>([]);
  const [breakSecondsLeft, setBreakSecondsLeft] = useState(BREAK_DURATION_SEC);
  const [breakTimer, setBreakTimer] = useState<ReturnType<typeof setInterval> | null>(null);
  const [summaryVisible, setSummaryVisible] = useState(false);

  const startWorkout = () => {
    setPhase('working');
    setCurrentSet(1);
    setCurrentRep(1);
    setCompletedSets([]);
    setSetLog([]);
  };

  const completeRep = () => {
    if (currentRep < totalReps) {
      setCurrentRep(r => r + 1);
    } else {
      const finishedSet: CompletedSet = { setNumber: currentSet, repsCompleted: totalReps };
      const newSetLog = [...setLog, finishedSet];
      setSetLog(newSetLog);
      const newCompleted = [...completedSets, currentSet];
      setCompletedSets(newCompleted);

      if (currentSet < totalSets) {
        setPhase('break');
        startBreakTimer();
        setCurrentRep(1);
        setCurrentSet(s => s + 1);
      } else {
        endWorkout(newSetLog);
      }
    }
  };

  const startBreakTimer = () => {
    setBreakSecondsLeft(BREAK_DURATION_SEC);
    const id = setInterval(() => {
      setBreakSecondsLeft(prev => {
        if (prev <= 1) { clearInterval(id); return 0; }
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

  const endWorkout = (finalSetLog?: CompletedSet[]) => {
    if (breakTimer) clearInterval(breakTimer);
    setBreakTimer(null);

    const log = finalSetLog ?? setLog;
    const logWithPartial =
      (phase === 'working' || phase === 'break') && currentRep > 1 && !finalSetLog
        ? [...log, { setNumber: currentSet, repsCompleted: currentRep - 1 }]
        : log;

    setSummaryLog(logWithPartial);

    if (logWithPartial.length > 0) {
      saveWorkout({
        exercise: 'Bicep Curl',
        sets: logWithPartial,
        totalReps: logWithPartial.reduce((sum, s) => sum + s.repsCompleted, 0),
      });
    }

    setPhase('done');
    setSummaryVisible(true);
  };

  const handleEndPress = () => {
    Alert.alert('End Workout', 'Are you sure you want to end this workout?', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'End', style: 'destructive', onPress: () => endWorkout() },
    ]);
  };

  const resetSession = () => {
    setSummaryVisible(false);
    setPhase('idle');
    setCurrentSet(1);
    setCurrentRep(1);
    setCompletedSets([]);
    setSetLog([]);
    setTotalSets(DEFAULT_SETS);
    setTotalReps(DEFAULT_REPS);
  };

  const setsCompleted = completedSets.length;
  const progressPercent =
    phase === 'working' || phase === 'break'
      ? ((setsCompleted * totalReps + (currentRep - 1)) /
      (totalSets * totalReps)) * 100
      : 0;

  if (phase === 'idle') {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>Live Session</Text>
        <View style={styles.centerContent}>
          <Text style={styles.exerciseLabel}>Bicep Curl</Text>
          <Text style={styles.idleSubLabel}>Configure your workout</Text>

          <View style={styles.stepperCard}>
            <Stepper
              label="Sets"
              value={totalSets}
              onDecrement={() => setTotalSets(s => Math.max(1, s - 1))}
              onIncrement={() => setTotalSets(s => s + 1)}
            />
            <View style={styles.stepperDivider} />
            <Stepper
              label="Reps"
              value={totalReps}
              onDecrement={() => setTotalReps(r => Math.max(1, r - 1))}
              onIncrement={() => setTotalReps(r => r + 1)}
            />
          </View>

          <Text style={styles.subLabel}>{totalSets} sets × {totalReps} reps</Text>

          <TouchableOpacity style={styles.startButton} onPress={startWorkout}>
            <Text style={styles.startButtonText}>Start Workout</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  if (phase === 'break') {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>Live Session</Text>
        <View style={styles.progressBarBg}>
          <View style={[styles.progressBarFill, { width: `${progressPercent}%` }]} />
        </View>
        <Text style={styles.progressText}>{setsCompleted} / {totalSets} sets complete</Text>
        <View style={styles.centerContent}>
          <Text style={styles.breakTitle}>Break Time</Text>
          <Text style={styles.breakTimer}>{breakSecondsLeft}s</Text>
          <Text style={styles.breakSub}>Up next — Set {currentSet} of {totalSets}</Text>
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
  if (phase === 'working') {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>Live Session</Text>
        <View style={styles.progressBarBg}>
          <View style={[styles.progressBarFill, { width: `${progressPercent}%` }]} />
        </View>
        <Text style={styles.progressText}>{setsCompleted} / {totalSets} sets complete</Text>
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
              <Text style={styles.statValue}>{totalReps}</Text>
              <Text style={styles.statLabel}>GOAL</Text>
            </View>
          </View>

          <View style={styles.repDots}>
            {Array.from({ length: totalReps }).map((_, i) => (
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
              {currentRep < totalReps ? `Complete Rep ${currentRep}` : 'Finish Set'}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.breakButtonAlt} onPress={() => { setPhase('break'); startBreakTimer(); }}>
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
              Sets Completed: {summaryLog.length} / {totalSets}
            </Text>
            <Text style={styles.summaryLine}>
              Total Reps: {summaryLog.reduce((sum, s) => sum + s.repsCompleted, 0)}
            </Text>
            {summaryLog.map(s => (
              <Text key={s.setNumber} style={styles.summarySetLine}>
                Set {s.setNumber}: {s.repsCompleted} reps
              </Text>
            ))}
            <TouchableOpacity style={styles.primaryButton} onPress={resetSession}>
              <Text style={styles.primaryButtonText}>Done</Text>
            </TouchableOpacity>
          </View>
        </View>
      </Modal>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: 'white' },
  title: { fontSize: 24, fontWeight: 'bold', padding: 20, paddingTop: 65, backgroundColor: 'white' },
  progressBarBg: { height: 6, backgroundColor: '#E0E0E0', marginHorizontal: 20, borderRadius: 3, marginTop: 4 },
  progressBarFill: { height: 6, backgroundColor: '#658e58', borderRadius: 3 },
  progressText: { fontSize: 12, color: '#888', textAlign: 'right', marginRight: 20, marginTop: 4 },
  centerContent: { flex: 1, justifyContent: 'center', alignItems: 'center', paddingHorizontal: 30 },
  exerciseLabel: { fontSize: 28, fontWeight: '700', marginBottom: 6 },
  idleSubLabel: { fontSize: 15, color: '#888', marginBottom: 24 },
  subLabel: { fontSize: 15, color: '#888', marginTop: 20 },
  statRow: {
    flexDirection: 'row', alignItems: 'center', backgroundColor: '#F7F7F7',
    borderRadius: 16, paddingVertical: 20, paddingHorizontal: 30,
    marginBottom: 24, width: '100%', justifyContent: 'space-around',
  },
  statBox: { alignItems: 'center', flex: 1 },
  statValue: { fontSize: 42, fontWeight: '800', color: '#1A1A1A' },
  statLabel: { fontSize: 11, fontWeight: '600', color: '#999', letterSpacing: 1.2, marginTop: 2 },
  divider: { width: 1, height: 50, backgroundColor: '#DDD' },
  repDots: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'center', marginBottom: 28, gap: 6 },
  dot: { width: 14, height: 14, borderRadius: 7, backgroundColor: '#E0E0E0', margin: 3 },
  dotDone: { backgroundColor: '#658e58' },
  dotCurrent: { backgroundColor: '#1A1A1A' },
  primaryButton: {
    backgroundColor: '#D6E0D3', paddingHorizontal: 40, paddingVertical: 16,
    borderRadius: 30, marginBottom: 12, borderWidth: 1, borderColor: '#636363',
    width: '100%', alignItems: 'center',
  },
  primaryButtonText: { color: 'black', fontSize: 16, fontWeight: '600' },
  breakButtonAlt: {
    backgroundColor: '#F1E5C4', paddingHorizontal: 40, paddingVertical: 16,
    borderRadius: 30, marginBottom: 12, borderWidth: 1, borderColor: '#636363',
    width: '100%', alignItems: 'center',
  },
  breakButtonAltText: { color: 'black', fontSize: 16, fontWeight: '600' },
  endButton: {
    paddingHorizontal: 40, paddingVertical: 14, borderRadius: 30,
    borderWidth: 1, borderColor: '#636363', width: '100%', alignItems: 'center',
    backgroundColor: '#DCBDBD',
  },
  endButtonText: { color: '#000000', fontSize: 16, fontWeight: '600' },
  startButton: {
    backgroundColor: '#D6E0D3', paddingHorizontal: 40, paddingVertical: 15,
    borderRadius: 30, marginTop: 20, borderWidth: 1, borderColor: '#636363',
  },
  startButtonText: { color: 'black', fontSize: 18, fontWeight: '500' },
  breakTitle: { fontSize: 32, fontWeight: '700', marginBottom: 10 },
  breakTimer: { fontSize: 64, fontWeight: '800', color: '#658e58', marginBottom: 6 },
  breakSub: { fontSize: 15, color: '#888', marginBottom: 36 },
  modalOverlay: { flex: 1, backgroundColor: 'rgba(0,0,0,0.4)', justifyContent: 'flex-end' },
  modalCard: { backgroundColor: 'white', borderTopLeftRadius: 24, borderTopRightRadius: 24, padding: 30, paddingBottom: 50 },
  modalTitle: { fontSize: 22, fontWeight: '700', marginBottom: 20 },
  summaryLine: { fontSize: 16, color: '#444', marginBottom: 8 },
  summarySetLine: { fontSize: 14, color: '#666', marginBottom: 4, paddingLeft: 12 },
  stepperCard: {
    width: '100%', backgroundColor: '#F7F7F7', borderRadius: 16,
    paddingVertical: 8, paddingHorizontal: 20, borderWidth: 1, borderColor: '#E0E0E0',
  },
  stepperRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', paddingVertical: 14 },
  stepperDivider: { height: 1, backgroundColor: '#E0E0E0' },
  stepperLabel: { fontSize: 16, fontWeight: '600', color: '#1A1A1A' },
  stepperControls: { flexDirection: 'row', alignItems: 'center', gap: 12 },
  stepperBtn: {
    width: 36, height: 36, borderRadius: 18, backgroundColor: '#D6E0D3',
    borderWidth: 1, borderColor: '#636363', alignItems: 'center', justifyContent: 'center',
  },
  stepperBtnDisabled: { backgroundColor: '#ECECEC', borderColor: '#C0C0C0' },
  stepperBtnText: { fontSize: 20, fontWeight: '600', color: '#1A1A1A', lineHeight: 22 },
  stepperValue: { fontSize: 22, fontWeight: '700', color: '#1A1A1A', minWidth: 32, textAlign: 'center' },
});

export default SessionScreen;