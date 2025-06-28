#pragma once
#include "fl/time_alpha.h"
#include "fl/math_macros.h"
#include "fl/fft.h"

/// Tracks a smoothed peak with attack, decay, and output-inertia time-constants.
class MaxFadeTracker {
public:
    /// @param attackTimeSec  τ₁: how quickly to rise toward a new peak.
    /// @param decayTimeSec   τ₂: how quickly to decay to 1/e of value.
    /// @param outputTimeSec  τ₃: how quickly the returned value follows currentLevel_.
    /// @param sampleRate     audio sample rate (e.g. 44100 or 48000).
    MaxFadeTracker(float attackTimeSec,
                   float decayTimeSec,
                   float outputTimeSec,
                   float sampleRate)
      : attackRate_(1.0f / attackTimeSec)
      , decayRate_(1.0f / decayTimeSec)
      , outputRate_(1.0f / outputTimeSec)
      , sampleRate_(sampleRate)
      , currentLevel_(0.0f)
      , smoothedOutput_(0.0f)
    {}

    void setAttackTime(float t){ attackRate_ = 1.0f/t; }
    void setDecayTime (float t){  decayRate_ = 1.0f/t; }
    void setOutputTime(float t){ outputRate_ = 1.0f/t; }

    /// Process one 512-sample block; returns [0…1] with inertia.
    float operator()(const int16_t* samples, size_t length) {
        assert(length == 512);
        // 1) block peak
        float peak = 0.0f;
        for (size_t i = 0; i < length; ++i) {
            float v = ABS(samples[i]) * (1.0f/32768.0f);
            peak = MAX(peak, v);
        }

        // 2) time delta
        float dt = static_cast<float>(length) / sampleRate_;

        // 3) update currentLevel_ with attack/decay
        if (peak > currentLevel_) {
            float riseFactor = 1.0f - fl::exp(-attackRate_ * dt);
            currentLevel_ += (peak - currentLevel_) * riseFactor;
        } else {
            float decayFactor = fl::exp(-decayRate_ * dt);
            currentLevel_ *= decayFactor;
        }

        // 4) output inertia: smooth smoothedOutput_ → currentLevel_
        float outFactor = 1.0f - fl::exp(-outputRate_ * dt);
        smoothedOutput_ += (currentLevel_ - smoothedOutput_) * outFactor;

        return smoothedOutput_;
    }

private:
    float attackRate_;    // = 1/τ₁
    float decayRate_;     // = 1/τ₂
    float outputRate_;    // = 1/τ₃
    float sampleRate_;
    float currentLevel_;  // instantaneous peak with attack/decay
    float smoothedOutput_; // returned value with inertia
};

/// Simple beat detector using energy and variance
class BeatDetector {
public:
    BeatDetector(float sensitivity = 1.5f, int historySize = 43)
        : sensitivity_(sensitivity)
        , historySize_(historySize)
        , historyIndex_(0)
        , beatHistory_(new float[historySize])
        , lastBeatTime_(0)
    {
        for (int i = 0; i < historySize_; ++i) {
            beatHistory_[i] = 0.0f;
        }
    }
    
    ~BeatDetector() {
        delete[] beatHistory_;
    }
    
    bool detectBeat(float energy, uint32_t currentTime) {
        // Add current energy to history
        beatHistory_[historyIndex_] = energy;
        historyIndex_ = (historyIndex_ + 1) % historySize_;
        
        // Calculate average energy
        float avgEnergy = 0.0f;
        for (int i = 0; i < historySize_; ++i) {
            avgEnergy += beatHistory_[i];
        }
        avgEnergy /= historySize_;
        
        // Calculate variance
        float variance = 0.0f;
        for (int i = 0; i < historySize_; ++i) {
            float diff = beatHistory_[i] - avgEnergy;
            variance += diff * diff;
        }
        variance /= historySize_;
        
        // Beat detection threshold
        float threshold = avgEnergy + (sensitivity_ * sqrt(variance));
        
        // Check if current energy exceeds threshold and enough time has passed
        if (energy > threshold && (currentTime - lastBeatTime_) > 100) {
            lastBeatTime_ = currentTime;
            return true;
        }
        
        return false;
    }
    
    void setSensitivity(float sensitivity) {
        sensitivity_ = sensitivity;
    }
    
private:
    float sensitivity_;
    int historySize_;
    int historyIndex_;
    float* beatHistory_;
    uint32_t lastBeatTime_;
};

// Commented out for now to avoid compilation issues
/*
/// Frequency band analyzer for more detailed audio analysis
class FrequencyBandAnalyzer {
public:
    static const int NUM_BANDS = 8;
    
    FrequencyBandAnalyzer() {
        // Initialize band boundaries (roughly octave bands)
        bandBoundaries_[0] = 0;      // 0-60 Hz (Sub-bass)
        bandBoundaries_[1] = 3;      // 60-250 Hz (Bass)
        bandBoundaries_[2] = 12;     // 250-500 Hz (Low-mid)
        bandBoundaries_[3] = 25;     // 500-2kHz (Mid)
        bandBoundaries_[4] = 50;     // 2-4kHz (High-mid)
        bandBoundaries_[5] = 100;    // 4-8kHz (High)
        bandBoundaries_[6] = 200;    // 8-16kHz (Very high)
        bandBoundaries_[7] = 400;    // 16kHz+ (Brilliance)
        bandBoundaries_[8] = 512;    // End
        
        for (int i = 0; i < NUM_BANDS; ++i) {
            bandEnergies_[i] = 0.0f;
            bandPeaks_[i] = 0.0f;
        }
    }
    
    void analyzeBands(const FFTBins& fft) {
        // Reset band energies
        for (int i = 0; i < NUM_BANDS; ++i) {
            bandEnergies_[i] = 0.0f;
        }
        
        // Calculate energy for each band
        for (int band = 0; band < NUM_BANDS; ++band) {
            int startBin = bandBoundaries_[band];
            int endBin = bandBoundaries_[band + 1];
            
            float energy = 0.0f;
            for (int bin = startBin; bin < endBin && bin < fft.bins_raw.size(); ++bin) {
                energy += fft.bins_raw[bin];
            }
            
            // Normalize by number of bins in band
            if (endBin > startBin) {
                energy /= (endBin - startBin);
            }
            
            bandEnergies_[band] = energy;
            
            // Update peak with decay
            if (energy > bandPeaks_[band]) {
                bandPeaks_[band] = energy;
            } else {
                bandPeaks_[band] *= 0.95f; // Decay
            }
        }
    }
    
    float getBandEnergy(int band) const {
        if (band >= 0 && band < NUM_BANDS) {
            return bandEnergies_[band];
        }
        return 0.0f;
    }
    
    float getBandPeak(int band) const {
        if (band >= 0 && band < NUM_BANDS) {
            return bandPeaks_[band];
        }
        return 0.0f;
    }
    
    float getNormalizedBandEnergy(int band) const {
        if (band >= 0 && band < NUM_BANDS && bandPeaks_[band] > 0) {
            return bandEnergies_[band] / bandPeaks_[band];
        }
        return 0.0f;
    }
    
private:
    int bandBoundaries_[NUM_BANDS + 1];
    float bandEnergies_[NUM_BANDS];
    float bandPeaks_[NUM_BANDS];
};
*/
