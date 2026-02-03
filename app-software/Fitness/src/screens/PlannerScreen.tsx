import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  Modal,
  TextInput,
  TouchableOpacity,
  Alert,
  ScrollView,
} from 'react-native';

type PlanType = 'workout' | 'break';

interface Plan {
  id: string;
  type: PlanType;
  name?: string;
  reps?: number;
  sets?: number;
  breakMinutes?: number;
}

const PlannerScreen: React.FC = () => {
  const [plans, setPlans] = useState<Plan[]>([]);
  const [modalVisible, setModalVisible] = useState(false);
  
  const [planType, setPlanType] = useState<PlanType>('workout');
  const [name, setName] = useState('');
  const [reps, setReps] = useState('');
  const [sets, setSets] = useState('');
  const [breakMinutes, setBreakMinutes] = useState('');

  const addPlan = () => {
    if (planType === 'workout' && (!name || !reps || !sets)) {
      Alert.alert('Please fill out workout name, reps, and sets.');
      return;
    }
    if (planType === 'break' && !breakMinutes) {
      Alert.alert('Please enter break duration.');
      return;
    }

    const newPlan: Plan = {
      id: Date.now().toString(),
      type: planType,
      name: planType === 'workout' ? name : undefined,
      reps: planType === 'workout' ? parseInt(reps) : undefined,
      sets: planType === 'workout' ? parseInt(sets) : undefined,
      breakMinutes: planType === 'break' ? parseInt(breakMinutes) : undefined,
    };
    setPlans([...plans, newPlan]);
    setModalVisible(false);
    // Reset form
    setName('');
    setReps('');
    setSets('');
    setBreakMinutes('');
    setPlanType('workout');
  };

  const renderPlanItem = ({ item }: { item: Plan }) => (
    <View style={styles.planCard}>
      {item.type === 'workout' ? (
        <Text style={styles.planText}>
          {item.name} — {item.reps} reps × {item.sets} sets
        </Text>
      ) : (
        <Text style={styles.planText}>Break — {item.breakMinutes} min</Text>
      )}
    </View>
  );
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Planner</Text>
      <ScrollView style={styles.content}>
        <TouchableOpacity
          style={styles.addButton}
          onPress={() => setModalVisible(true)}
        >
        <Text style={styles.addButtonText}>+ Add Workout Plan</Text>
        </TouchableOpacity>
        <Text style={styles.placeholder}>
          No plans available. Create a new plan above!
        </Text>
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
  content: {
    flex: 1,
    padding: 20,
  },
  placeholder: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
    marginTop: 50,
  },
  addButton: {
    backgroundColor: '#D6E0D3',
    paddingVertical: 30,
    borderRadius: 8,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#636363',
  },
  addButtonText: {
    color: 'black',
    fontWeight: '600',
    fontSize: 16,
    textAlign: 'center',
  },
  planCard: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 8,
    marginBottom: 10,
    borderWidth: 1,
    borderColor: '#ddd',
  },
  planText: {
    fontSize: 14,
    fontWeight: '500',
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.3)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  modalContainer: {
    width: '85%',
    backgroundColor: 'white',
    borderRadius: 8,
    padding: 20,
  },
  modalTitle: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 15,
  },
  typeContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 15,
  },
  typeButton: {
    flex: 1,
    paddingVertical: 10,
    borderWidth: 1,
    borderColor: '#aaa',
    borderRadius: 6,
    marginHorizontal: 5,
    alignItems: 'center',
  },
  selectedType: {
    backgroundColor: '#4CAF50',
    borderColor: '#4CAF50',
  },
  input: {
    borderWidth: 1,
    borderColor: '#aaa',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 8,
    marginBottom: 10,
  },
  saveButton: {
    backgroundColor: '#4CAF50',
    paddingVertical: 12,
    borderRadius: 6,
  },
  saveButtonText: {
    color: 'white',
    fontWeight: '600',
    textAlign: 'center',
  },
});

export default PlannerScreen;