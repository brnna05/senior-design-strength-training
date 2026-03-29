import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Modal,
  TextInput,
  FlatList,
  Alert,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { RootStackParamList } from '../navigation/RootNavigator';
import { useWorkouts, CompletedWorkout } from '../context/WorkoutContext';
import { useBLE } from '../context/BLEContext';

type DeleteMode = 'menu' | 'all' | 'range' | 'specific';

const SettingsScreen = () => {
  const navigation = useNavigation<NativeStackNavigationProp<RootStackParamList>>();
  const { workouts, deleteAllWorkouts, deleteWorkoutsByDateRange, deleteWorkoutById } = useWorkouts();

  const [modalVisible, setModalVisible] = useState(false);
  const [deleteMode, setDeleteMode] = useState<DeleteMode>('menu');
  const [startDate, setStartDate] = useState('');
  const [endDate, setEndDate] = useState('');
  const [rangeError, setRangeError] = useState('');
  const { scanForDevice, isConnected, connectedDeviceName } = useBLE();
  const [isScanning, setIsScanning] = useState(false);

  // ── helpers ────────────────────────────────────────────────

  const openModal = () => {
    setDeleteMode('menu');
    setModalVisible(true);
  };

  const closeModal = () => {
    setModalVisible(false);
    setStartDate('');
    setEndDate('');
    setRangeError('');
  };

  const isValidDate = (d: string) => /^\d{4}-\d{2}-\d{2}$/.test(d);

  const handleConnectDevice = () => {
    if (isConnected) {
      Alert.alert('Already Connected', 'A device is already connected.');
      return;
    }
    setIsScanning(true);
    scanForDevice();
    // Stop showing "scanning" after 10 seconds if nothing found
    setTimeout(() => setIsScanning(false), 10000);
  };

  // ── delete handlers ────────────────────────────────────────

  const handleDeleteAll = () => {
    Alert.alert(
      'Delete All Workouts',
      `Permanently delete all ${workouts.length} workout${workouts.length !== 1 ? 's' : ''}? This cannot be undone.`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete All',
          style: 'destructive',
          onPress: async () => {
            await deleteAllWorkouts();
            closeModal();
          },
        },
      ]
    );
  };

  const handleDeleteRange = async () => {
    setRangeError('');
    if (!isValidDate(startDate) || !isValidDate(endDate)) {
      setRangeError('Use YYYY-MM-DD format for both dates.');
      return;
    }
    if (startDate > endDate) {
      setRangeError('Start date must be on or before end date.');
      return;
    }
    const count = workouts.filter(w => w.date >= startDate && w.date <= endDate).length;
    if (count === 0) {
      setRangeError('No workouts found in that range.');
      return;
    }
    Alert.alert(
      'Delete Workouts',
      `Delete ${count} workout${count !== 1 ? 's' : ''} between ${startDate} and ${endDate}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            await deleteWorkoutsByDateRange(startDate, endDate);
            closeModal();
          },
        },
      ]
    );
  };

  const handleDeleteOne = (workout: CompletedWorkout) => {
    Alert.alert(
      'Delete Workout',
      `Delete "${workout.exercise}" on ${workout.date}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: () => deleteWorkoutById(workout.id),
        },
      ]
    );
  };

  // ── modal sub-views ────────────────────────────────────────

  const renderMenu = () => (
    <View>
      <TouchableOpacity style={styles.menuOption} onPress={() => setDeleteMode('all')}>
        <View style={styles.menuOptionText}>
          <Text style={styles.menuOptionTitle}>Delete All Workouts</Text>
          <Text style={styles.menuOptionDesc}>Remove every saved workout</Text>
        </View>
        <Text style={styles.chevron}>›</Text>
      </TouchableOpacity>

      <TouchableOpacity style={styles.menuOption} onPress={() => setDeleteMode('range')}>
        <View style={styles.menuOptionText}>
          <Text style={styles.menuOptionTitle}>Delete by Date Range</Text>
          <Text style={styles.menuOptionDesc}>Remove workouts between two dates</Text>
        </View>
        <Text style={styles.chevron}>›</Text>
      </TouchableOpacity>

      <TouchableOpacity style={[styles.menuOption, { borderBottomWidth: 0 }]} onPress={() => setDeleteMode('specific')}>
        <View style={styles.menuOptionText}>
          <Text style={styles.menuOptionTitle}>Delete a Specific Workout</Text>
          <Text style={styles.menuOptionDesc}>Browse and remove individual entries</Text>
        </View>
        <Text style={styles.chevron}>›</Text>
      </TouchableOpacity>
    </View>
  );

  const renderDeleteAll = () => (
    <View style={styles.subView}>
      <Text style={styles.subDesc}>
        Permanently remove every saved workout. This cannot be undone.
      </Text>
      <Text style={styles.bigCount}>
        {workouts.length} workout{workouts.length !== 1 ? 's' : ''} stored
      </Text>
      <TouchableOpacity
        style={[styles.deleteBtn, workouts.length === 0 && styles.deleteBtnDisabled]}
        onPress={handleDeleteAll}
        disabled={workouts.length === 0}
      >
        <Text style={styles.deleteBtnText}>Delete All Workouts</Text>
      </TouchableOpacity>
    </View>
  );

  const renderDeleteRange = () => (
    <View style={styles.subView}>
      <Text style={styles.subDesc}>
        Delete all workouts that fall within a date range.
      </Text>
      <Text style={styles.inputLabel}>Start Date</Text>
      <TextInput
        style={styles.dateInput}
        placeholder="YYYY-MM-DD"
        placeholderTextColor="#aaa"
        value={startDate}
        onChangeText={t => { setStartDate(t); setRangeError(''); }}
        maxLength={10}
        keyboardType="numbers-and-punctuation"
      />
      <Text style={styles.inputLabel}>End Date</Text>
      <TextInput
        style={styles.dateInput}
        placeholder="YYYY-MM-DD"
        placeholderTextColor="#aaa"
        value={endDate}
        onChangeText={t => { setEndDate(t); setRangeError(''); }}
        maxLength={10}
        keyboardType="numbers-and-punctuation"
      />
      {rangeError ? <Text style={styles.errorText}>{rangeError}</Text> : null}
      <TouchableOpacity style={styles.deleteBtn} onPress={handleDeleteRange}>
        <Text style={styles.deleteBtnText}>Delete Workouts in Range</Text>
      </TouchableOpacity>
    </View>
  );

  const renderDeleteSpecific = () => (
    <View style={styles.subView}>
      {workouts.length === 0 ? (
        <Text style={styles.emptyText}>No workouts saved yet.</Text>
      ) : (
        <FlatList
          data={workouts}
          keyExtractor={item => item.id}
          style={{ maxHeight: 460 }}
          renderItem={({ item }) => (
            <View style={styles.workoutRow}>
              <View style={{ flex: 1 }}>
                <Text style={styles.workoutExercise}>{item.exercise}</Text>
                <Text style={styles.workoutMeta}>
                  {item.date} · {item.totalReps} reps · {item.sets.length} sets
                </Text>
              </View>
              <TouchableOpacity style={styles.rowDeleteBtn} onPress={() => handleDeleteOne(item)}>
                <Text style={styles.rowDeleteBtnText}>Delete</Text>
              </TouchableOpacity>
            </View>
          )}
          ItemSeparatorComponent={() => <View style={{ height: 8 }} />}
        />
      )}
    </View>
  );

  const modalTitle: Record<DeleteMode, string> = {
    menu: 'Data Management',
    all: 'Delete All Workouts',
    range: 'Delete by Date Range',
    specific: 'Delete a Specific Workout',
  };

  const renderModalContent = () => {
    if (deleteMode === 'all') return renderDeleteAll();
    if (deleteMode === 'range') return renderDeleteRange();
    if (deleteMode === 'specific') return renderDeleteSpecific();
    return renderMenu();
  };

  // ── main render ────────────────────────────────────────────

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <ScrollView style={styles.content}>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Account</Text>

          <TouchableOpacity style={styles.row} onPress={() => {}}>
            <Text style={styles.rowLabel}>Profile</Text>
            <Text style={styles.chevron}>›</Text>
          </TouchableOpacity>

          <TouchableOpacity style={styles.row} onPress={openModal}>
            <Text style={styles.rowLabel}>Data Management</Text>
            <Text style={styles.chevron}>›</Text>
          </TouchableOpacity>

          <TouchableOpacity style={styles.row} onPress={() => navigation.replace('Login')}>
            <Text style={[styles.rowLabel, { color: '#FF3B30' }]}>Log Out</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Bluetooth</Text>
          <TouchableOpacity style={styles.row} onPress={handleConnectDevice}>
            <Text style={styles.rowLabel}>
              {isScanning ? 'Scanning...' : 'Connect Device'}
            </Text>
            <Text style={[styles.rowValue, isConnected && { color: '#658e58' }]}>
              {isConnected ? '● Connected' : isScanning ? '● Scanning' : 'Not Connected'}
            </Text>
          </TouchableOpacity>
        </View>

      </ScrollView>

      {/* ── Data Management Modal ── */}
      <Modal
        visible={modalVisible}
        animationType="slide"
        presentationStyle="pageSheet"
        onRequestClose={closeModal}
      >
        <View style={styles.modalContainer}>
          <View style={styles.modalHeader}>
            {deleteMode !== 'menu' ? (
              <TouchableOpacity onPress={() => setDeleteMode('menu')}>
                <Text style={styles.modalNavBtn}>‹ Back</Text>
              </TouchableOpacity>
            ) : (
              <View style={{ width: 60 }} />
            )}
            <Text style={styles.modalTitle}>{modalTitle[deleteMode]}</Text>
            <TouchableOpacity onPress={closeModal}>
              <Text style={[styles.modalNavBtn, { textAlign: 'right' }]}>Done</Text>
            </TouchableOpacity>
          </View>

          <View style={{ flex: 1 }}>{renderModalContent()}</View>
        </View>
      </Modal>
    </View>
  );
};

// ── styles ─────────────────────────────────────────────────────────────────

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    padding: 20,
    paddingTop: 65,
    backgroundColor: 'white',
  },
  content: { flex: 1 },

  section: { backgroundColor: 'white', marginTop: 20, paddingVertical: 10 },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#666',
    paddingHorizontal: 20,
    paddingVertical: 10,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 20,
    paddingVertical: 15,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  rowLabel: { fontSize: 16, color: '#000' },
  rowValue: { fontSize: 14, color: '#666' },
  chevron: { fontSize: 20, color: '#ccc' },

  modalContainer: { flex: 1, backgroundColor: '#f5f5f5' },
  modalHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 16,
    paddingVertical: 16,
    backgroundColor: 'white',
    borderBottomWidth: 1,
    borderBottomColor: '#e0e0e0',
  },
  modalTitle: { fontSize: 17, fontWeight: '600', flex: 1, textAlign: 'center' },
  modalNavBtn: { fontSize: 17, color: '#007AFF', width: 60 },

  menuOption: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: 'white',
    paddingHorizontal: 20,
    paddingVertical: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
    marginTop: 20,
  },
  menuOptionIcon: { fontSize: 22, marginRight: 14 },
  menuOptionText: { flex: 1 },
  menuOptionTitle: { fontSize: 16, color: '#000' },
  menuOptionDesc: { fontSize: 12, color: '#999', marginTop: 2 },

  subView: { padding: 20 },
  subDesc: { fontSize: 14, color: '#666', lineHeight: 20, marginBottom: 16 },
  bigCount: {
    fontSize: 32,
    fontWeight: '700',
    textAlign: 'center',
    color: '#333',
    marginBottom: 24,
  },

  inputLabel: { fontSize: 13, color: '#666', marginTop: 14, marginBottom: 6 },
  dateInput: {
    backgroundColor: 'white',
    borderRadius: 10,
    paddingHorizontal: 14,
    paddingVertical: 12,
    fontSize: 16,
    borderWidth: 1,
    borderColor: '#e0e0e0',
    color: '#000',
  },
  errorText: { color: '#FF3B30', fontSize: 13, marginTop: 10 },

  deleteBtn: {
    backgroundColor: '#FF3B30',
    borderRadius: 12,
    paddingVertical: 15,
    alignItems: 'center',
    marginTop: 24,
  },
  deleteBtnDisabled: { backgroundColor: '#ccc' },
  deleteBtnText: { color: 'white', fontSize: 16, fontWeight: '600' },

  workoutRow: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: 'white',
    paddingHorizontal: 16,
    paddingVertical: 14,
    borderRadius: 10,
  },
  workoutExercise: { fontSize: 15, fontWeight: '500', color: '#000' },
  workoutMeta: { fontSize: 12, color: '#999', marginTop: 3 },
  rowDeleteBtn: {
    backgroundColor: '#FF3B30',
    borderRadius: 8,
    paddingHorizontal: 14,
    paddingVertical: 8,
  },
  rowDeleteBtnText: { color: 'white', fontSize: 14, fontWeight: '600' },
  emptyText: { fontSize: 15, color: '#999', textAlign: 'center', marginTop: 40 },
});

export default SettingsScreen;