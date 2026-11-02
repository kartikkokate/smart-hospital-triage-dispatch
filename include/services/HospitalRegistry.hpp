#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstddef>
#include <algorithm>

#include "models/Patient.hpp"
#include "ds/HashTableChaining.hpp"
#include "ds/BinarySearchTree.hpp"
#include "ds/BinaryMinHeap.hpp" // For kMaxSupportedHandle

/**
 * @brief Authoritative Primary Patient Registry.
 * 
 * DESIGN RATIONALE:
 * - Single source of truth: Owns all authoritative Patient objects via std::unique_ptr<Patient>.
 * - Prevents dual-ownership or stale duplicate record anomalies.
 * - Indexes:
 *     1. HashTableChaining<std::string, Patient*>: Expected O(1) primary lookup by patientId.
 *     2. BinarySearchTree<std::string, Patient*>: Hierarchical BST index supporting in-order
 *        traversal, range queries, and tree metric inspections.
 *     3. Dense handle array: Fast O(1) mapping from internalHandle (0 to 99999) to Patient*.
 * - Guarantees non-owning pointers remain stable across registry lifecycle.
 */
class HospitalRegistry {
private:
    std::vector<std::unique_ptr<Patient>> patientStore_;
    HashTableChaining<std::string, Patient*> idHashTable_{17, 0.75};
    BinarySearchTree<std::string, Patient*> idBST_;
    std::vector<Patient*> handleMap_;

    bool simulateAllocationFailure_{false};
    bool simulateFailureAfterHash_{false};

public:
    HospitalRegistry() = default;

    // Test seam to verify exception-safety rollback on allocation failure
    void setSimulateAllocationFailure(bool enable) noexcept {
        simulateAllocationFailure_ = enable;
    }

    void setSimulateFailureAfterHash(bool enable) noexcept {
        simulateFailureAfterHash_ = enable;
    }

    void clear() noexcept {
        patientStore_.clear();
        idHashTable_.clear();
        idBST_.clear();
        handleMap_.clear();
    }

    // Non-copyable to prevent duplicating authoritative ownership
    HospitalRegistry(const HospitalRegistry&) = delete;
    HospitalRegistry& operator=(const HospitalRegistry&) = delete;

    // Move semantics supported
    HospitalRegistry(HospitalRegistry&&) noexcept = default;
    HospitalRegistry& operator=(HospitalRegistry&&) noexcept = default;

    /**
     * @brief Registers an authoritative patient record into the hospital system.
     *        Provides strong exception safety: either all storage and indexes commit,
     *        or all mutations are rolled back cleanly with zero dangling references.
     * 
     * @param patient Unique pointer transferring ownership of Patient.
     * @param outError Descriptive error message on failure.
     * @return true if successfully registered; false on duplicate ID, invalid handle, or null pointer.
     */
    bool registerPatient(std::unique_ptr<Patient> patient, std::string& outError) {
        if (!patient) {
            outError = "Cannot register null patient pointer.";
            return false;
        }

        if (patient->patientId.empty()) {
            outError = "Patient ID cannot be empty.";
            return false;
        }

        // Validate uniqueness via primary hash table
        if (idHashTable_.search(patient->patientId) != nullptr) {
            outError = "Duplicate patient ID: " + patient->patientId + " is already registered.";
            return false;
        }

        // Validate handle bounds
        if (patient->internalHandle < 0 || patient->internalHandle >= TriageMinHeap::kMaxSupportedHandle) {
            outError = "Patient handle " + std::to_string(patient->internalHandle) + 
                       " is out of valid range [0, " + std::to_string(TriageMinHeap::kMaxSupportedHandle - 1) + "].";
            return false;
        }

        size_t handleIdx = static_cast<size_t>(patient->internalHandle);
        if (handleIdx < handleMap_.size() && handleMap_[handleIdx] != nullptr) {
            outError = "Duplicate internal handle " + std::to_string(patient->internalHandle) + 
                       " is already assigned to " + handleMap_[handleIdx]->patientId + ".";
            return false;
        }

        // Pre-reserve capacity in vector storage
        try {
            if (handleIdx >= handleMap_.size()) {
                handleMap_.resize(handleIdx + 1, nullptr);
            }
            patientStore_.reserve(patientStore_.size() + 1);
        } catch (...) {
            outError = "Memory allocation failure reserving registry storage.";
            return false;
        }

        Patient* rawPtr = patient.get();
        std::string pid = patient->patientId;
        bool bstInserted = false;
        bool handleSet = false;

        try {
            if (simulateAllocationFailure_) {
                throw std::bad_alloc();
            }

            idHashTable_.insert(pid, rawPtr);

            if (simulateFailureAfterHash_) {
                throw std::bad_alloc();
            }

            idBST_.insert(pid, rawPtr);
            bstInserted = true;

            handleMap_[handleIdx] = rawPtr;
            handleSet = true;

            patientStore_.push_back(std::move(patient));
        } catch (...) {
            // Strictly rollback all partial mutations
            if (handleSet) {
                handleMap_[handleIdx] = nullptr;
            }
            if (bstInserted) {
                idBST_.remove(pid);
            }
            // Unconditionally remove from hash table to guarantee no partially published or dangling entry remains
            idHashTable_.remove(pid);
            outError = "Memory allocation failure during patient registration; all registry indexes rolled back cleanly.";
            return false;
        }

        return true;
    }

    /**
     * @brief Constant-time primary lookup by alphanumeric patient ID.
     */
    Patient* findPatient(const std::string& patientId) const {
        if (patientId.empty()) return nullptr;
        Patient* const* res = idHashTable_.search(patientId);
        return res ? *res : nullptr;
    }

    /**
     * @brief O(1) direct lookup by stable dense integer handle.
     */
    Patient* findPatientByHandle(int handle) const noexcept {
        if (handle < 0 || static_cast<size_t>(handle) >= handleMap_.size()) {
            return nullptr;
        }
        return handleMap_[static_cast<size_t>(handle)];
    }

    /**
     * @brief Checks if patient exists in the registry.
     */
    bool hasPatient(const std::string& patientId) const {
        return findPatient(patientId) != nullptr;
    }

    /**
     * @brief Returns total count of registered patients.
     */
    size_t getPatientCount() const noexcept {
        return patientStore_.size();
    }

    /**
     * @brief Returns non-owning pointers to all registered patients in registration order.
     */
    std::vector<Patient*> getAllPatients() const {
        std::vector<Patient*> result;
        result.reserve(patientStore_.size());
        for (const auto& p : patientStore_) {
            result.push_back(p.get());
        }
        return result;
    }

    /**
     * @brief Exposes the primary BST index for hierarchical inspections and tree metrics.
     */
    const BinarySearchTree<std::string, Patient*>& getBSTIndex() const noexcept {
        return idBST_;
    }

    /**
     * @brief Exposes the primary Hash Table for capacity, load factor, and bucket inspections.
     */
    const HashTableChaining<std::string, Patient*>& getHashTable() const noexcept {
        return idHashTable_;
    }
};
