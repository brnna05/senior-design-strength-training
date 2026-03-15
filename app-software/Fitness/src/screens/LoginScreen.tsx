import React, { useState } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  KeyboardAvoidingView,
  Platform,
  ActivityIndicator,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { RootStackParamList } from '../navigation/RootNavigator';

const LoginScreen = () => {
  const navigation = useNavigation<NativeStackNavigationProp<RootStackParamList>>();
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  
  const handleLogin = async () => {
    setError('');
    if (!email || !password) {
      setError('Please enter your email and password.');
      return;
    }
    if (!email.includes('@')) {
      setError('Please enter a valid email address.');
      return;
    }
    setLoading(true);

    setTimeout(() => {
      setLoading(false);
      setError('Invalid email or password.');
      navigation.replace('Main');
    }, 1500);
    
  };

  return (
    <KeyboardAvoidingView
      style={styles.container}
      behavior={Platform.OS === 'ios' ? 'padding' : 'height'}>
      <View style={styles.inner}>

        {/* Header */}
        <View style={styles.header}>
          <Text style={styles.appName}>Strength Training</Text>
        </View>

        {/* Form */}
        <View style={styles.form}>
          <Text style={styles.label}>Email Address</Text>
          <TextInput
            style={styles.input}
            placeholder="Email Address"
            placeholderTextColor="#AAAAAA"
            value={email}
            onChangeText={setEmail}
            keyboardType="email-address"
            autoCapitalize="none"
            autoCorrect={false}
          />

          <Text style={styles.label}>Password</Text>
          <View style={styles.passwordRow}>
            <TextInput
              style={styles.passwordInput}
              placeholder="Password"
              placeholderTextColor="#AAAAAA"
              value={password}
              onChangeText={setPassword}
              secureTextEntry={!showPassword}
              autoCapitalize="none"
            />
            <TouchableOpacity
              style={styles.showBtn}
              onPress={() => setShowPassword(v => !v)}>
              <Text style={styles.showBtnText}>
                {showPassword ? 'Hide' : 'Show'}
              </Text>
            </TouchableOpacity>
          </View>

          <TouchableOpacity style={styles.forgotBtn}>
            <Text style={styles.forgotText}>Forgot password?</Text>
          </TouchableOpacity>

          {error ? <Text style={styles.errorText}>{error}</Text> : null}

          <TouchableOpacity
            style={[styles.loginBtn, loading && styles.loginBtnDisabled]}
            onPress={handleLogin}
            disabled={loading}>
            {loading
              ? <ActivityIndicator color="white" />
              : <Text style={styles.loginBtnText}>Log In</Text>
            }
          </TouchableOpacity>
        </View>

        {/* Footer */}
        <View style={styles.footer}>
          <Text style={styles.footerText}>Don't have an account? </Text>
          <TouchableOpacity>
            <Text style={styles.signupLink}>Sign up</Text>
          </TouchableOpacity>
        </View>

      </View>
    </KeyboardAvoidingView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: 'white' },
  inner: { flex: 1, justifyContent: 'center', paddingHorizontal: 28 },

  header: { alignItems: 'center', marginBottom: 40 },
  appName: { fontSize: 28, fontWeight: '800', color: '#1A1A1A', marginBottom: 4 },
  tagline: { fontSize: 13, color: '#888' },

  form: {
    backgroundColor: 'white', borderRadius: 16, padding: 24,
    borderWidth: 1, borderColor: '#acacac', marginBottom: 20,
  },
  label: { fontSize: 13, fontWeight: '600', color: '#444', marginBottom: 6, marginTop: 4 },
  input: {
    borderWidth: 1, borderColor: '#acacac', borderRadius: 10,
    paddingHorizontal: 14, paddingVertical: 12, fontSize: 15,
    color: '#1A1A1A', backgroundColor: '#FAFAFA', marginBottom: 16,
  },
  passwordRow: {
    flexDirection: 'row', alignItems: 'center', borderWidth: 1,
    borderColor: '#acacac', borderRadius: 10, backgroundColor: '#FAFAFA', marginBottom: 8,
  },
  passwordInput: {
    flex: 1, paddingHorizontal: 14, paddingVertical: 12, fontSize: 15, color: '#1A1A1A',
  },
  showBtn: { paddingHorizontal: 14, paddingVertical: 12 },
  showBtnText: { fontSize: 13, fontWeight: '600', color: '#658e58' },
  forgotBtn: { alignSelf: 'flex-end', marginBottom: 20 },
  forgotText: { fontSize: 13, color: '#658e58', fontWeight: '500' },
  errorText: {
    fontSize: 13, color: '#8B2020', backgroundColor: '#FDF5F5',
    borderWidth: 1, borderColor: '#DCBDBD', borderRadius: 8,
    paddingHorizontal: 12, paddingVertical: 8, marginBottom: 14,
  },
  loginBtn: {
    backgroundColor: '#658e58', paddingVertical: 15, borderRadius: 30, alignItems: 'center',
  },
  loginBtnDisabled: { opacity: 0.6 },
  loginBtnText: { color: 'white', fontSize: 16, fontWeight: '700' },

  footer: { flexDirection: 'row', justifyContent: 'center' },
  footerText: { fontSize: 14, color: '#888' },
  signupLink: { fontSize: 14, color: '#658e58', fontWeight: '600' },
});

export default LoginScreen;