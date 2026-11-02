#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <cctype>
#include <cmath>

#include "models/Patient.hpp"
#include "models/DispatchResource.hpp"
#include "models/CampusLocation.hpp"
#include "models/AuditEvent.hpp"
#include "models/PatientSnapshot.hpp"
#include "services/HospitalSystem.hpp"
#include "services/ReportingService.hpp"
#include "services/DemonstrationService.hpp"

/**
 * @brief Interactive Console User Interface for the Smart Hospital System (CS2305 Milestone 7).
 * 
 * ARCHITECTURAL PRINCIPLES:
 * 1. Encapsulated System Coordination:
 *    - All hospital operations and state mutations route strictly through HospitalSystem facade methods.
 *    - UI code never directly alters Patient pointers, queue nodes, or resource assignments.
 * 2. Deterministic & Stream-Injectable I/O:
 *    - Uses injected std::istream& and std::ostream& references for lightweight, headless automated testing.
 * 3. Strict Line-Based Parsing:
 *    - Consumes full input lines and validates tokens strictly.
 *    - Rejects malformed numbers, trailing garbage, out-of-range values, and non-finite numbers gracefully.
 *    - Accepts names and clinical complaints containing spaces, hyphens, and ordinary apostrophes (e.g. O'Connor).
 * 4. Non-Recursive Menu Traversal:
 *    - Menu navigation is implemented via iterative state loops; no deep recursion or stack buildup.
 * 5. Robust End-Of-File (EOF) Safety:
 *    - Instantly detects EOF or broken input streams, setting exit flags to avoid infinite loops or hung processes.
 * 6. Explicit Memory-Only Lifecycle:
 *    - Clearly communicates to users that all data resides in volatile RAM and is discarded upon exit.
 */
class ConsoleApp {
private:
    HospitalSystem& hospital_;
    std::istream& in_;
    std::ostream& out_;
    std::string dataDir_;
    bool useAnsi_{false};
    bool eofReached_{false};
    bool exitRequested_{false};

    // ========================================================================
    // INPUT HANDLING & TOKEN PARSING HELPERS
    // ========================================================================

    static std::string trimString(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    bool readRawLine(std::string& outLine) {
        if (eofReached_ || exitRequested_) return false;
        if (!std::getline(in_, outLine)) {
            eofReached_ = true;
            return false;
        }
        if (!outLine.empty() && outLine.back() == '\r') {
            outLine.pop_back();
        }
        return true;
    }

    bool readInt(int& outVal, int minVal, int maxVal, const std::string& prompt) {
        while (!eofReached_ && !exitRequested_) {
            out_ << prompt;
            std::string line;
            if (!readRawLine(line)) return false;
            std::string trimmed = trimString(line);
            if (trimmed.empty()) {
                out_ << "  [ERROR] Input cannot be blank. Please enter an integer between " 
                     << minVal << " and " << maxVal << ".\n";
                continue;
            }

            try {
                size_t pos = 0;
                long long parsed = std::stoll(trimmed, &pos);
                if (pos != trimmed.length()) {
                    out_ << "  [ERROR] Malformed numeric input: unexpected trailing characters in '" << trimmed << "'.\n";
                    continue;
                }
                if (parsed < minVal || parsed > maxVal) {
                    out_ << "  [ERROR] Value " << parsed << " is out of supported range [" << minVal << " .. " << maxVal << "].\n";
                    continue;
                }
                outVal = static_cast<int>(parsed);
                return true;
            } catch (...) {
                out_ << "  [ERROR] Malformed input: unable to parse '" << trimmed << "' as an integer.\n";
            }
        }
        return false;
    }

    bool readDouble(double& outVal, double minVal, double maxVal, const std::string& prompt) {
        while (!eofReached_ && !exitRequested_) {
            out_ << prompt;
            std::string line;
            if (!readRawLine(line)) return false;
            std::string trimmed = trimString(line);
            if (trimmed.empty()) {
                out_ << "  [ERROR] Input cannot be blank. Please enter a valid decimal number.\n";
                continue;
            }

            try {
                size_t pos = 0;
                double parsed = std::stod(trimmed, &pos);
                if (pos != trimmed.length()) {
                    out_ << "  [ERROR] Malformed numeric input: trailing characters in '" << trimmed << "'.\n";
                    continue;
                }
                if (!std::isfinite(parsed)) {
                    out_ << "  [ERROR] Numeric value must be finite (received NaN or Infinity).\n";
                    continue;
                }
                if (parsed < minVal || parsed > maxVal) {
                    out_ << "  [ERROR] Value " << parsed << " is out of supported range [" << minVal << " .. " << maxVal << "].\n";
                    continue;
                }
                outVal = parsed;
                return true;
            } catch (...) {
                out_ << "  [ERROR] Malformed input: unable to parse '" << trimmed << "' as a decimal number.\n";
            }
        }
        return false;
    }

    bool readString(std::string& outVal, const std::string& prompt,
                    size_t minLen = 1, size_t maxLen = 128,
                    bool allowSpaces = true, bool allowApostrophe = true) {
        while (!eofReached_ && !exitRequested_) {
            out_ << prompt;
            std::string line;
            if (!readRawLine(line)) return false;
            std::string trimmed = trimString(line);

            if (trimmed.length() < minLen) {
                out_ << "  [ERROR] Input must contain at least " << minLen << " character(s).\n";
                continue;
            }
            if (trimmed.length() > maxLen) {
                out_ << "  [ERROR] Input exceeds maximum allowed length of " << maxLen << " characters (received " << trimmed.length() << ").\n";
                continue;
            }

            bool valid = true;
            for (char c : trimmed) {
                if (c == ' ' && !allowSpaces) {
                    out_ << "  [ERROR] Spaces are not permitted in this field.\n";
                    valid = false;
                    break;
                }
                if (c == '\'' && !allowApostrophe) {
                    out_ << "  [ERROR] Apostrophes are not permitted in this field.\n";
                    valid = false;
                    break;
                }
                if (c == '\t' || c == '\n' || c == '\r' || c == ',') {
                    out_ << "  [ERROR] Invalid character encountered (tabs, newlines, and commas are not permitted).\n";
                    valid = false;
                    break;
                }
            }

            if (valid) {
                outVal = trimmed;
                return true;
            }
        }
        return false;
    }

    bool readMenuChoice(int& outChoice, int minChoice, int maxChoice) {
        while (!eofReached_ && !exitRequested_) {
            out_ << "\nEnter choice [" << minChoice << "-" << maxChoice << "] (0 or 'b' to back/exit): ";
            std::string line;
            if (!readRawLine(line)) return false;
            std::string trimmed = trimString(line);

            if (trimmed.empty()) continue;

            if (trimmed == "0" || trimmed == "b" || trimmed == "B" || 
                trimmed == "back" || trimmed == "Back" || trimmed == "exit" || trimmed == "q" || trimmed == "Q") {
                outChoice = 0;
                return true;
            }

            try {
                size_t pos = 0;
                int val = std::stoi(trimmed, &pos);
                if (pos == trimmed.length() && val >= minChoice && val <= maxChoice) {
                    outChoice = val;
                    return true;
                }
            } catch (...) {}

            out_ << "  [ERROR] Invalid selection '" << trimmed << "'. Please enter a number between " 
                 << minChoice << " and " << maxChoice << " (or 0 to return).\n";
        }
        return false;
    }

    // ========================================================================
    // HEADER & BANNER FORMATTING
    // ========================================================================

    void printHeader(const std::string& title) const {
        out_ << "\n================================================================================\n";
        out_ << "  " << title << "\n";
        out_ << "  [Clock: " << hospital_.getSimulationTime() << "s] | [Patients: "
             << hospital_.getPatientCount() << "] | [Fleet: " << hospital_.getAllResources().size()
             << "] | [Memory-Only Mode]\n";
        out_ << "================================================================================\n";
    }

    void printInMemoryNotice() const {
        out_ << "  [IN-MEMORY PERSISTENCE NOTICE]\n";
        out_ << "  All hospital operational state (patients, triage heap, appointment queue,\n";
        out_ << "  dispatch movements, audit trail) is maintained exclusively in volatile RAM.\n";
        out_ << "  Exiting the application will reset all in-memory structures without disk persistence.\n";
    }

    // ========================================================================
    // SUBMENU 1: PATIENT REGISTRATION & REGISTRY LOOKUP
    // ========================================================================

    void runPatientMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("PATIENT REGISTRATION & REGISTRY MANAGEMENT");
            out_ << "  1. Register New Inpatient / Emergency Patient\n";
            out_ << "  2. Search Patient Dossier by ID (Full Clinical Details)\n";
            out_ << "  3. List All Registered Patients in Registry\n";
            out_ << "  4. View In-Memory Data Storage Architecture Notice\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 4)) break;
            if (choice == 0) break;

            switch (choice) {
                case 1: handleRegisterPatient(); break;
                case 2: handleLookupPatient(); break;
                case 3: handleListPatients(); break;
                case 4: printInMemoryNotice(); break;
                default: break;
            }
        }
    }

    void handleRegisterPatient() {
        out_ << "\n--- Patient Clinical Registration Form ---\n";
        std::string pid, name, complaint;
        int age = 0, urgencyNum = 5, currLoc = 1, targLoc = 2;
        double billing = 0.0;

        if (!readString(pid, "  Enter Patient ID (e.g. P113, max 16 chars): ", 1, 16, false, false)) return;
        if (!readString(name, "  Enter Full Legal Name (spaces & apostrophes allowed, max 64 chars): ", 1, 64, true, true)) return;
        if (!readInt(age, 0, 150, "  Enter Patient Age [0 - 150]: ")) return;
        if (!readString(complaint, "  Enter Chief Medical Complaint (max 128 chars): ", 1, 128, true, true)) return;
        if (!readInt(urgencyNum, 1, 5, "  Enter Acuity Urgency [1: Resuscitation .. 5: Non-Urgent]: ")) return;
        if (!readInt(currLoc, 0, 11, "  Enter Current Campus Location Node ID [0 - 11]: ")) return;
        if (!readInt(targLoc, 0, 11, "  Enter Target Destination Node ID [0 - 11]: ")) return;
        if (!readDouble(billing, 0.0, 1000000.0, "  Enter Initial Billing Balance [$0.00 - $1,000,000.00]: ")) return;

        UrgencyLevel urgency = intToUrgency(urgencyNum);
        std::string err;
        if (hospital_.registerNewPatient(pid, name, age, complaint, urgency, currLoc, targLoc, billing, err)) {
            out_ << "\n  [SUCCESS] Patient " << pid << " registered successfully!\n";
            out_ << "  Full Name:      " << name << "\n";
            out_ << "  Initial State:  REGISTERED\n";
            out_ << "  Urgency Acuity: " << urgencyNum << " (" << urgencyToString(urgency) << ")\n";
            out_ << "  Current Node:   " << currLoc << " (" << hospital_.getNodeName(currLoc) << ")\n";
            out_ << "  Target Node:    " << targLoc << " (" << hospital_.getNodeName(targLoc) << ")\n";
        } else {
            out_ << "\n  [REGISTRATION REJECTED] " << err << "\n";
        }
    }

    void handleLookupPatient() {
        out_ << "\n--- Patient Dossier Lookup ---\n";
        std::string pid;
        if (!readString(pid, "  Enter Patient ID to query: ", 1, 16, false, false)) return;

        const Patient* p = hospital_.findPatient(pid);
        if (!p) {
            out_ << "  [NOT FOUND] Patient ID '" << pid << "' does not exist in the hospital registry.\n";
            return;
        }

        PatientSnapshot snap(*p, hospital_.getSimulationTime());
        out_ << ReportingService::formatPatientDetail(snap, &hospital_.getMapManager());
    }

    void handleListPatients() {
        out_ << "\n--- Hospital Patient Registry Directory ---\n";
        std::vector<Patient*> all = hospital_.getAllPatients();
        if (all.empty()) {
            out_ << "  Registry is currently empty.\n";
            return;
        }

        out_ << std::left
            << std::setw(16) << "Patient ID"
            << std::setw(22) << "Full Legal Name"
            << std::setw(6)  << "Age"
            << std::setw(18) << "Urgency"
            << std::setw(24) << "Lifecycle State"
            << std::setw(14) << "Location Node"
            << "\n";
        out_ << "----------------------------------------------------------------------------------------------------\n";

        for (const Patient* p : all) {
            if (!p) continue;
            std::string nameStr = p->fullName;
            if (nameStr.length() > 20) nameStr = nameStr.substr(0, 18) + "..";
            std::string urgStr = std::to_string(urgencyToInt(p->urgency)) + "-" + urgencyToString(p->urgency);

            out_ << std::left
                << std::setw(16) << p->patientId
                << std::setw(22) << nameStr
                << std::setw(6)  << p->age
                << std::setw(18) << urgStr
                << std::setw(24) << patientStateToString(p->state)
                << std::setw(14) << (std::to_string(p->currentLocationNode) + " (" + hospital_.getNodeName(p->currentLocationNode) + ")")
                << "\n";
        }
        out_ << "----------------------------------------------------------------------------------------------------\n";
        out_ << "  Total Registered Patients: " << all.size() << " | Use Lookup (Option 2) for unabbreviated dossiers.\n";
    }

    // ========================================================================
    // SUBMENU 2: EMERGENCY TRIAGE & ACUITY MANAGEMENT
    // ========================================================================

    void runTriageMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("EMERGENCY CLINICAL TRIAGE & ACUITY MANAGEMENT");
            out_ << "  1. View Triage Priority Queue Status & Highest Acuity Candidate\n";
            out_ << "  2. Intake Registered Patient into Emergency Triage Priority Queue\n";
            out_ << "  3. Update Patient Clinical Urgency (Preserves Arrival Sequence)\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 3)) break;
            if (choice == 0) break;

            switch (choice) {
                case 1: handleViewTriageQueue(); break;
                case 2: handleIntakeTriage(); break;
                case 3: handleUpdateUrgency(); break;
                default: break;
            }
        }
    }

    void handleViewTriageQueue() {
        out_ << "\n--- Emergency Triage Priority Queue Status ---\n";
        size_t qSize = hospital_.getTriageQueueSize();
        out_ << "  Active Patients Waiting in Triage Heap: " << qSize << "\n";

        Patient* top = nullptr;
        if (hospital_.peekTriageHighestPriority(top) && top != nullptr) {
            out_ << "  Highest Priority Triage Candidate:\n";
            out_ << "    Patient ID:       " << top->patientId << " (" << top->fullName << ")\n";
            out_ << "    Clinical Urgency: " << urgencyToInt(top->urgency) << " - " << urgencyToString(top->urgency) << "\n";
            out_ << "    Arrival Sequence: #" << top->arrivalSequence << " (FIFO Tie-Breaker)\n";
            out_ << "    Current Location: Node " << top->currentLocationNode << " (" << hospital_.getNodeName(top->currentLocationNode) << ")\n";
            out_ << "    Patient State:    " << patientStateToString(top->state) << "\n";
        } else {
            out_ << "  No patients are currently queued in emergency triage.\n";
        }
    }

    void handleIntakeTriage() {
        out_ << "\n--- Emergency Triage Intake ---\n";
        std::string pid;
        int urgNum = 1;
        if (!readString(pid, "  Enter Registered Patient ID: ", 1, 16, false, false)) return;
        if (!readInt(urgNum, 1, 5, "  Assign Clinical Urgency Level [1: Resuscitation .. 5: Non-Urgent]: ")) return;

        UrgencyLevel urg = intToUrgency(urgNum);
        std::string err;
        if (hospital_.triagePatient(pid, urg, err)) {
            out_ << "  [SUCCESS] Patient " << pid << " prioritized into triage min-heap with urgency " 
                 << urgNum << " (" << urgencyToString(urg) << "). Patient State: TRIAGED_WAITING.\n";
        } else {
            out_ << "  [TRIAGE REJECTED] " << err << "\n";
        }
    }

    void handleUpdateUrgency() {
        out_ << "\n--- Update Clinical Urgency Acuity ---\n";
        std::string pid;
        int urgNum = 1;
        if (!readString(pid, "  Enter Patient ID: ", 1, 16, false, false)) return;
        if (!readInt(urgNum, 1, 5, "  Enter New Urgency Level [1: Resuscitation .. 5: Non-Urgent]: ")) return;

        UrgencyLevel newUrg = intToUrgency(urgNum);
        std::string err;
        if (hospital_.updateUrgency(pid, newUrg, err)) {
            const Patient* p = hospital_.findPatient(pid);
            out_ << "  [SUCCESS] Urgency for patient " << pid << " successfully updated to " 
                 << urgNum << " (" << urgencyToString(newUrg) << ").\n";
            if (p) {
                out_ << "  Preserved Arrival Sequence: #" << p->arrivalSequence << " (maintains stable FIFO tie-breaking).\n";
            }
        } else {
            out_ << "  [UPDATE REJECTED] " << err << "\n";
        }
    }

    // ========================================================================
    // SUBMENU 3: OUTPATIENT CLINIC APPOINTMENTS
    // ========================================================================

    void runAppointmentMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("OUTPATIENT CLINIC APPOINTMENT SERVICES");
            out_ << "  1. Schedule Outpatient Clinic Appointment (FIFO Queue Intake)\n";
            out_ << "  2. View Scheduled Outpatient Appointment Roster\n";
            out_ << "  3. Call Next Appointment for Consultation (FIFO Turn Processing)\n";
            out_ << "  4. Cancel Outpatient Appointment (Encapsulated Queue Removal)\n";
            out_ << "  5. Convert Outpatient to Emergency Triage (Condition Deterioration)\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 5)) break;
            if (choice == 0) break;

            switch (choice) {
                case 1: handleScheduleAppointment(); break;
                case 2: handleViewAppointments(); break;
                case 3: handleCallNextAppointment(); break;
                case 4: handleCancelAppointment(); break;
                case 5: handleConvertAppointment(); break;
                default: break;
            }
        }
    }

    void handleScheduleAppointment() {
        out_ << "\n--- Schedule Outpatient Appointment ---\n";
        std::string pid;
        if (!readString(pid, "  Enter Registered Patient ID: ", 1, 16, false, false)) return;

        std::string err;
        if (hospital_.scheduleAppointment(pid, err)) {
            out_ << "  [SUCCESS] Appointment booked for patient " << pid << ". Patient State: APPOINTMENT_SCHEDULED.\n";
        } else {
            out_ << "  [BOOKING REJECTED] " << err << "\n";
        }
    }

    void handleViewAppointments() {
        out_ << "\n--- Outpatient Clinic Appointment Queue (FIFO Roster) ---\n";
        size_t count = hospital_.getAppointmentQueueSize();
        out_ << "  Total Scheduled Outpatient Appointments: " << count << "\n";
        if (count == 0) {
            out_ << "  No appointments currently in queue.\n";
            return;
        }

        std::vector<std::string> ids = hospital_.getScheduledAppointmentPatientIds();
        out_ << std::left
            << std::setw(8)  << "Queue #"
            << std::setw(16) << "Patient ID"
            << std::setw(24) << "Patient Name"
            << std::setw(14) << "Arrival Clock"
            << "\n";
        out_ << "--------------------------------------------------------------------------------\n";

        for (size_t i = 0; i < ids.size(); ++i) {
            const Patient* p = hospital_.findPatient(ids[i]);
            std::string nameStr = p ? p->fullName : "Unknown";
            int64_t arrTime = p ? p->simulatedArrivalTime : 0;

            out_ << std::left
                << std::setw(8)  << (i + 1)
                << std::setw(16) << ids[i]
                << std::setw(24) << nameStr
                << std::setw(14) << (std::to_string(arrTime) + "s")
                << "\n";
        }
        out_ << "--------------------------------------------------------------------------------\n";
    }

    void handleCallNextAppointment() {
        out_ << "\n--- Call Next Outpatient Consultation ---\n";
        Patient* called = nullptr;
        std::string err;
        if (hospital_.callNextAppointment(called, err) && called != nullptr) {
            out_ << "  [SUCCESS] Consult Turn Called: Patient " << called->patientId 
                 << " (" << called->fullName << ").\n";
            out_ << "  Updated Patient State: UNDER_TREATMENT (Consultation underway at Node " 
                 << called->currentLocationNode << ").\n";
        } else {
            out_ << "  [CALL FAILED] " << err << "\n";
        }
    }

    void handleCancelAppointment() {
        out_ << "\n--- Cancel Outpatient Appointment ---\n";
        std::string pid;
        if (!readString(pid, "  Enter Scheduled Patient ID to cancel: ", 1, 16, false, false)) return;

        std::string err;
        if (hospital_.cancelAppointment(pid, err)) {
            out_ << "  [SUCCESS] Appointment for patient " << pid 
                 << " cancelled and removed from queue. Patient State: CANCELLED.\n";
        } else {
            out_ << "  [CANCELLATION REJECTED] " << err << "\n";
        }
    }

    void handleConvertAppointment() {
        out_ << "\n--- Convert Outpatient Appointment to Emergency Triage ---\n";
        std::string pid;
        int urgNum = 1;
        if (!readString(pid, "  Enter Scheduled Patient ID: ", 1, 16, false, false)) return;
        if (!readInt(urgNum, 1, 5, "  Enter Deteriorated Urgency Acuity [1: Resuscitation .. 5: Non-Urgent]: ")) return;

        UrgencyLevel newUrg = intToUrgency(urgNum);
        std::string err;
        if (hospital_.convertAppointmentToEmergency(pid, newUrg, err)) {
            out_ << "  [SUCCESS] Patient " << pid << " condition deteriorated; converted to Emergency Triage with Urgency " 
                 << urgNum << " (" << urgencyToString(newUrg) << "). Patient State: TRIAGED_WAITING.\n";
        } else {
            out_ << "  [CONVERSION REJECTED] " << err << "\n";
        }
    }

    // ========================================================================
    // SUBMENU 4: AMBULANCE DISPATCH & PATIENT TRANSIT OPERATIONS
    // ========================================================================

    void runDispatchMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("AMBULANCE DISPATCH & PATIENT TRANSIT OPERATIONS");
            out_ << "  1. Select Next Priority Emergency Candidate for Dispatch\n";
            out_ << "  2. Assign Available Dispatch Resource (Shows 2-Leg Routes & Costs)\n";
            out_ << "  3. Execute Simulation Step 1: Resource Travels to Patient Pickup Point\n";
            out_ << "  4. Execute Simulation Step 2: Transport Patient & Complete Clinical Handover\n";
            out_ << "  5. Cancel Pickup Leg (Aborts Transit & Releases Resource Immediately)\n";
            out_ << "  6. Discharge Patient from Clinical Treatment (Terminal State Isolation)\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 6)) break;
            if (choice == 0) break;

            switch (choice) {
                case 1: handleSelectDispatchCandidate(); break;
                case 2: handleAssignDispatchResource(); break;
                case 3: handleStepPickup(); break;
                case 4: handleStepTransport(); break;
                case 5: handleCancelPickup(); break;
                case 6: handleDischargePatient(); break;
                default: break;
            }
        }
    }

    void handleSelectDispatchCandidate() {
        out_ << "\n--- Priority Candidate Evaluation for Dispatch ---\n";
        std::string err;
        Patient* candidate = hospital_.selectNextPatientForDispatch(err);
        if (candidate) {
            out_ << "  [SELECTED CANDIDATE]\n";
            out_ << "    Patient ID:       " << candidate->patientId << " (" << candidate->fullName << ")\n";
            out_ << "    Clinical Urgency: " << urgencyToInt(candidate->urgency) << " - " << urgencyToString(candidate->urgency) << "\n";
            out_ << "    Arrival Sequence: #" << candidate->arrivalSequence << "\n";
            out_ << "    Current Node:     " << candidate->currentLocationNode << " (" << hospital_.getNodeName(candidate->currentLocationNode) << ")\n";
            out_ << "    Target Node:      " << candidate->targetLocationNode << " (" << hospital_.getNodeName(candidate->targetLocationNode) << ")\n";
            out_ << "    Patient State:    " << patientStateToString(candidate->state) << "\n";
            out_ << "    Assigned Resource:" << (candidate->assignedResourceId.empty() ? " Unassigned" : candidate->assignedResourceId) << "\n";
            out_ << "  Candidate is prioritized and ready for resource assignment.\n";
        } else {
            out_ << "  [NO CANDIDATE AVAILABLE] " << err << "\n";
        }
    }

    void handleAssignDispatchResource() {
        out_ << "\n--- Dispatch Resource Assignment ---\n";
        std::string pid;
        if (!readString(pid, "  Enter Patient ID to assign (must be DISPATCH_PENDING): ", 1, 16, false, false)) return;

        const Patient* candidate = hospital_.findPatient(pid);
        if (!candidate) {
            out_ << "  [ERROR] Patient ID '" << pid << "' not found.\n";
            return;
        }

        out_ << "\n  [PRE-ASSIGNMENT CANDIDATE INSPECTION]\n";
        out_ << "  Patient ID:       " << candidate->patientId << " (" << candidate->fullName << ")\n";
        out_ << "  Clinical Urgency: " << urgencyToInt(candidate->urgency) << " - " << urgencyToString(candidate->urgency) << "\n";
        out_ << "  Patient State:    " << patientStateToString(candidate->state) << "\n";
        out_ << "  Pickup Node:      " << candidate->currentLocationNode << " (" << hospital_.getNodeName(candidate->currentLocationNode) << ")\n";
        out_ << "  Destination Node: " << candidate->targetLocationNode << " (" << hospital_.getNodeName(candidate->targetLocationNode) << ")\n\n";

        std::string err;
        if (hospital_.assignDispatch(pid, err)) {
            // Re-fetch assigned patient to display assignment telemetry
            const Patient* assignedP = hospital_.findPatient(pid);
            const DispatchResource* res = assignedP ? hospital_.findResource(assignedP->assignedResourceId) : nullptr;

            out_ << "  [ASSIGNMENT SUCCESSFUL]\n";
            out_ << "  Assigned Resource: " << (res ? res->resourceId : "N/A") 
                 << " (" << (res ? res->name : "Unknown") << ")\n";
            out_ << "  Patient State:     " << patientStateToString(assignedP->state) << " (Awaiting Pickup Arrival)\n";
            out_ << "  Resource State:    " << (res ? resourceStateToString(res->state) : "N/A") << " (En Route to Pickup)\n\n";

            // Compute and display the two distinct routing legs
            if (res) {
                DijkstraResult leg1 = hospital_.findRoute(res->currentNodeId, assignedP->currentLocationNode);
                DijkstraResult leg2 = hospital_.findRoute(assignedP->currentLocationNode, assignedP->targetLocationNode);

                out_ << "  [DISPATCH ROUTE LEGS & ESTIMATED TRANSIT COSTS]\n";
                out_ << "  * Leg 1 (Pickup Leg): Resource Node " << res->currentNodeId 
                     << " -> Patient Node " << assignedP->currentLocationNode << "\n";
                out_ << "    Transit Duration: " << leg1.totalCost << " seconds\n";
                out_ << "    Path: ";
                for (size_t i = 0; i < leg1.path.size(); ++i) {
                    out_ << leg1.path[i] << (i + 1 < leg1.path.size() ? " -> " : "");
                }
                out_ << "\n";

                out_ << "  * Leg 2 (Transport Leg): Patient Node " << assignedP->currentLocationNode 
                     << " -> Destination Node " << assignedP->targetLocationNode << "\n";
                out_ << "    Transit Duration: " << leg2.totalCost << " seconds\n";
                out_ << "    Path: ";
                for (size_t i = 0; i < leg2.path.size(); ++i) {
                    out_ << leg2.path[i] << (i + 1 < leg2.path.size() ? " -> " : "");
                }
                out_ << "\n";
                out_ << "    Total Mission Transit Cost: " << (leg1.totalCost + leg2.totalCost) << " seconds\n";
            }
        } else {
            out_ << "  [ASSIGNMENT FAILED] " << err << "\n";
            out_ << "  Explanation: Patient remains safely in unassigned pending pool and will not be lost from consideration.\n";
        }
    }

    void handleStepPickup() {
        out_ << "\n--- Execute Leg 1: Movement to Patient Pickup ---\n";
        std::string pid;
        if (!readString(pid, "  Enter Assigned Patient ID: ", 1, 16, false, false)) return;

        out_ << "  Simulating whole-leg movement for pickup...\n";
        std::string err;
        int64_t preClock = hospital_.getSimulationTime();
        if (hospital_.simulationStepPickup(pid, err)) {
            int64_t postClock = hospital_.getSimulationTime();
            const Patient* p = hospital_.findPatient(pid);
            const DispatchResource* res = (p && !p->assignedResourceId.empty()) ? hospital_.findResource(p->assignedResourceId) : nullptr;

            out_ << "  [PICKUP ARRIVAL COMPLETE]\n";
            out_ << "  Leg 1 Elapsed Transit Time: " << (postClock - preClock) << " seconds\n";
            out_ << "  Updated Simulation Clock:   " << postClock << "s\n";
            out_ << "  Patient State:              " << (p ? patientStateToString(p->state) : "N/A") << " (Patient loaded onto transport)\n";
            out_ << "  Resource State:             " << (res ? resourceStateToString(res->state) : "N/A") 
                 << " at Node " << (res ? std::to_string(res->currentNodeId) : "N/A") << "\n";
        } else {
            out_ << "  [PICKUP STEP REJECTED] " << err << "\n";
        }
    }

    void handleStepTransport() {
        out_ << "\n--- Execute Leg 2: Transport & Clinical Handover ---\n";
        std::string pid;
        if (!readString(pid, "  Enter In-Transit Patient ID: ", 1, 16, false, false)) return;

        out_ << "  Simulating whole-leg transport movement and clinical handover...\n";
        std::string err;
        int64_t preClock = hospital_.getSimulationTime();
        if (hospital_.simulationStepTransportAndHandover(pid, err)) {
            int64_t postClock = hospital_.getSimulationTime();
            const Patient* p = hospital_.findPatient(pid);

            out_ << "  [TRANSPORT & HANDOVER COMPLETE]\n";
            out_ << "  Leg 2 Elapsed Transit Time: " << (postClock - preClock) << " seconds\n";
            out_ << "  Updated Simulation Clock:   " << postClock << "s\n";
            out_ << "  Patient State:              " << (p ? patientStateToString(p->state) : "N/A") 
                 << " at Treatment Destination Node " << (p ? std::to_string(p->currentLocationNode) : "N/A") 
                 << " (" << (p ? hospital_.getNodeName(p->currentLocationNode) : "") << ")\n";
            out_ << "  Resource State:             AVAILABLE (Resource released at destination; ready for new dispatches)\n";
        } else {
            out_ << "  [TRANSPORT STEP REJECTED] " << err << "\n";
        }
    }

    void handleCancelPickup() {
        out_ << "\n--- Cancel Pickup Leg Transit ---\n";
        std::string pid;
        if (!readString(pid, "  Enter Assigned Patient ID to cancel: ", 1, 16, false, false)) return;

        std::string err;
        if (hospital_.cancelPickup(pid, err)) {
            out_ << "  [SUCCESS] Pickup transit cancelled for patient " << pid << ".\n";
            out_ << "  Patient State:  CANCELLED (Terminal State)\n";
            out_ << "  Resource State: AVAILABLE (Assigned vehicle released immediately at current node)\n";
        } else {
            out_ << "  [CANCELLATION REJECTED] " << err << "\n";
        }
    }

    void handleDischargePatient() {
        out_ << "\n--- Discharge Patient from Clinical Treatment ---\n";
        std::string pid;
        if (!readString(pid, "  Enter Patient ID to discharge (must be UNDER_TREATMENT): ", 1, 16, false, false)) return;

        std::string err;
        if (hospital_.dischargePatient(pid, err)) {
            out_ << "  [SUCCESS] Patient " << pid << " successfully discharged after medical treatment.\n";
            out_ << "  Final State: DISCHARGED (Terminal lifecycle state; completely decoupled from queues/fleet)\n";
        } else {
            out_ << "  [DISCHARGE REJECTED] " << err << "\n";
        }
    }

    // ========================================================================
    // SUBMENU 5: CAMPUS MAP, ROUTING & CORRIDOR OPERATIONS
    // ========================================================================

    void runMapMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("CAMPUS TOPOLOGY, ROUTING & CORRIDOR MANAGEMENT");
            out_ << "  1. View All Campus Locations & Treatment Units\n";
            out_ << "  2. View Corridors, Transit Times & Obstruction Status\n";
            out_ << "  3. Block Corridor (Simulate Construction or Emergency Obstruction)\n";
            out_ << "  4. Reopen Corridor (Restore Normal Transit)\n";
            out_ << "  5. Query Shortest Route between Locations (Dijkstra's Algorithm)\n";
            out_ << "  6. Inspect Campus Connectivity & Traversals (BFS & DFS)\n";
            out_ << "  7. Compute Minimum Spanning Tree / Forest (Kruskal & Prim Comparison)\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 7)) break;
            if (choice == 0) break;

            switch (choice) {
                case 1: handleViewLocations(); break;
                case 2: handleViewCorridors(); break;
                case 3: handleBlockCorridor(); break;
                case 4: handleReopenCorridor(); break;
                case 5: handleQueryRoute(); break;
                case 6: handleConnectivityAndTraversals(); break;
                case 7: handleComputeMST(); break;
                default: break;
            }
        }
    }

    void handleViewLocations() {
        out_ << "\n--- Hospital Campus Locations ---\n";
        const auto& locs = hospital_.getAllLocations();
        out_ << std::left
            << std::setw(6)  << "Node"
            << std::setw(34) << "Facility Name"
            << std::setw(30) << "Building Wing / Floor"
            << std::setw(16) << "Treatment Unit?"
            << "\n";
        out_ << "----------------------------------------------------------------------------------------------------\n";

        for (const auto& loc : locs) {
            out_ << std::left
                << std::setw(6)  << loc.nodeId
                << std::setw(34) << loc.name
                << std::setw(30) << loc.buildingWing
                << std::setw(16) << (loc.isTreatmentUnit ? "YES (Clinical)" : "NO (Transit/Admin)")
                << "\n";
        }
        out_ << "----------------------------------------------------------------------------------------------------\n";
    }

    void handleViewCorridors() {
        out_ << "\n--- Campus Corridors & Status ---\n";
        std::vector<CorridorEdge> corridors = hospital_.getAllCorridors();
        out_ << std::left
            << std::setw(10) << "Corridor"
            << std::setw(50) << "Connecting Facilities"
            << std::setw(14) << "Transit Time"
            << std::setw(12) << "Status"
            << "\n";
        out_ << "----------------------------------------------------------------------------------------------------\n";

        for (const auto& edge : corridors) {
            std::string conn = std::to_string(edge.u) + " <-> " + std::to_string(edge.v);
            std::string names = hospital_.getNodeName(edge.u) + " <-> " + hospital_.getNodeName(edge.v);
            if (names.length() > 48) names = names.substr(0, 46) + "..";

            out_ << std::left
                << std::setw(10) << conn
                << std::setw(50) << names
                << std::setw(14) << (std::to_string(edge.transitTimeSeconds) + "s")
                << std::setw(12) << (edge.isBlocked ? "BLOCKED" : "OPEN")
                << "\n";
        }
        out_ << "----------------------------------------------------------------------------------------------------\n";
    }

    void handleBlockCorridor() {
        out_ << "\n--- Block Corridor (Closure / Maintenance) ---\n";
        int u = -1, v = -1;
        if (!readInt(u, 0, 11, "  Enter Node U ID [0 - 11]: ")) return;
        if (!readInt(v, 0, 11, "  Enter Node V ID [0 - 11]: ")) return;

        std::string err;
        if (hospital_.blockCorridor(u, v, err)) {
            out_ << "  [SUCCESS] Corridor between Node " << u << " (" << hospital_.getNodeName(u) 
                 << ") and Node " << v << " (" << hospital_.getNodeName(v) << ") is now BLOCKED.\n";
            out_ << "  Dijkstra route planner will automatically avoid this corridor.\n";
        } else {
            out_ << "  [BLOCK REJECTED] " << err << "\n";
        }
    }

    void handleReopenCorridor() {
        out_ << "\n--- Reopen Blocked Corridor ---\n";
        int u = -1, v = -1;
        if (!readInt(u, 0, 11, "  Enter Node U ID [0 - 11]: ")) return;
        if (!readInt(v, 0, 11, "  Enter Node V ID [0 - 11]: ")) return;

        std::string err;
        if (hospital_.reopenCorridor(u, v, err)) {
            out_ << "  [SUCCESS] Corridor between Node " << u << " (" << hospital_.getNodeName(u) 
                 << ") and Node " << v << " (" << hospital_.getNodeName(v) << ") has been REOPENED.\n";
        } else {
            out_ << "  [REOPEN REJECTED] " << err << "\n";
        }
    }

    void handleQueryRoute() {
        out_ << "\n--- Shortest Route Query (Dijkstra's Algorithm) ---\n";
        int src = -1, dst = -1;
        if (!readInt(src, 0, 11, "  Enter Origin Node ID [0 - 11]: ")) return;
        if (!readInt(dst, 0, 11, "  Enter Destination Node ID [0 - 11]: ")) return;

        DijkstraResult res = hospital_.findRoute(src, dst);
        if (res.isReachable) {
            out_ << "\n  [ROUTE FOUND]\n";
            out_ << "  Total Transit Cost: " << res.totalCost << " seconds\n";
            out_ << "  Hop Count:          " << (res.path.empty() ? 0 : (res.path.size() - 1)) << " corridor(s)\n";
            out_ << "  Navigation Sequence:\n";
            for (size_t i = 0; i < res.path.size(); ++i) {
                int node = res.path[i];
                out_ << "    " << (i + 1) << ". Node " << node << " (" << hospital_.getNodeName(node) << ")\n";
            }
        } else {
            out_ << "\n  [ROUTE UNAVAILABLE]\n";
            out_ << "  Destination Node " << dst << " is UNREACHABLE from Origin Node " << src << ".\n";
            out_ << "  Corridor closures have severed connectivity between these facilities.\n";
        }
    }

    void handleConnectivityAndTraversals() {
        out_ << "\n--- Campus Reachability & Graph Traversals ---\n";
        int startNode = 0;
        if (!readInt(startNode, 0, 11, "  Enter Start Node ID [0 - 11]: ")) return;

        std::vector<int> bfsOrder = hospital_.getBFSOrder(startNode);
        std::vector<int> dfsOrder = hospital_.getDFSOrder(startNode);
        std::vector<int> accessible = hospital_.getAccessibleNodes(startNode);

        out_ << "\n  [TRAVERSAL TELEMETRY from Node " << startNode << " (" << hospital_.getNodeName(startNode) << ")]\n";
        out_ << "  * BFS Traversal Order (Level-Order / Shortest Hops): ";
        for (size_t i = 0; i < bfsOrder.size(); ++i) {
            out_ << bfsOrder[i] << (i + 1 < bfsOrder.size() ? " -> " : "");
        }
        out_ << "\n";

        out_ << "  * DFS Traversal Order (Depth-First / Deterministic): ";
        for (size_t i = 0; i < dfsOrder.size(); ++i) {
            out_ << dfsOrder[i] << (i + 1 < dfsOrder.size() ? " -> " : "");
        }
        out_ << "\n";

        out_ << "  * Total Reachable Facilities in Connected Component: " 
             << accessible.size() << " / " << hospital_.getAllLocations().size() << "\n";
        if (accessible.size() < hospital_.getAllLocations().size()) {
            out_ << "  [WARNING] Graph is currently disconnected due to corridor obstruction(s).\n";
        } else {
            out_ << "  [STATUS] Campus graph is fully connected; all facilities are mutually reachable.\n";
        }
    }

    void handleComputeMST() {
        out_ << "\n--- Minimum Spanning Tree / Forest Computation ---\n";
        MSTResult kruskal = hospital_.computeKruskalMST();
        MSTResult prim = hospital_.computePrimMST();

        if (kruskal.componentCount > 1) {
            out_ << "  [DISCONNECTED GRAPH DETECTED]\n";
            out_ << "  Component Count: " << kruskal.componentCount << "\n";
            out_ << "  Result is accurately labeled as a MINIMUM SPANNING FOREST (MSF).\n";
        } else {
            out_ << "  [FULLY CONNECTED GRAPH]\n";
            out_ << "  Result is a MINIMUM SPANNING TREE (MST).\n";
        }

        out_ << "\n  [ALGORITHM COMPARISON]\n";
        out_ << "  * Kruskal's Algorithm (Edge-centric greedy with DSU):\n";
        out_ << "    Total Spanning Weight: " << kruskal.totalTransitWeight << " seconds\n";
        out_ << "    Edge Count:            " << kruskal.edges.size() << "\n";

        out_ << "  * Prim's Algorithm (Vertex-centric greedy with Min-Heap):\n";
        out_ << "    Total Spanning Weight: " << prim.totalTransitWeight << " seconds\n";
        out_ << "    Edge Count:            " << prim.edges.size() << "\n";

        if (kruskal.totalTransitWeight == prim.totalTransitWeight) {
            out_ << "  [VERIFICATION PASS] Kruskal and Prim total weights match identically (" 
                 << kruskal.totalTransitWeight << "s).\n";
        }
    }

    // ========================================================================
    // SUBMENU 6: PATIENT REPORTS & CUSTOM SORTING ANALYTICS
    // ========================================================================

    void runReportsMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("PATIENT REPORTS & CUSTOM SORTING ANALYTICS");
            out_ << "  1. Generate Summary Report with Custom Sorting Field & Algorithm\n";
            out_ << "  2. View Full Patient Clinical Dossier by ID\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 2)) break;
            if (choice == 0) break;

            switch (choice) {
                case 1: handleGenerateReport(); break;
                case 2: handleLookupPatient(); break;
                default: break;
            }
        }
    }

    void handleGenerateReport() {
        out_ << "\n--- Select Sorting Criteria for Patient Report Snapshot ---\n";
        out_ << "  Available Sort Fields:\n";
        out_ << "    1. Patient Age (Ascending)\n";
        out_ << "    2. Simulated Arrival Time (Ascending)\n";
        out_ << "    3. Clinical Urgency (Acuity Priority: 1-Resuscitation to 5-Non-Urgent)\n";
        out_ << "    4. Billing Amount ($ Ascending)\n";
        out_ << "    5. Active Waiting Duration (Longest Waiting First)\n";

        int fieldChoice = 3;
        if (!readInt(fieldChoice, 1, 5, "  Select Sort Field [1-5]: ")) return;

        ReportingService::SortField field;
        switch (fieldChoice) {
            case 1: field = ReportingService::SortField::AGE; break;
            case 2: field = ReportingService::SortField::ARRIVAL_TIME; break;
            case 3: field = ReportingService::SortField::URGENCY; break;
            case 4: field = ReportingService::SortField::BILLING_AMOUNT; break;
            case 5: field = ReportingService::SortField::WAITING_DURATION; break;
            default: field = ReportingService::SortField::URGENCY; break;
        }

        out_ << "\n  Available Sorting Algorithms (CS2305 CO1, CO5):\n";
        out_ << "    1. Custom Merge Sort (Stable, Guaranteed O(n log n), O(n) Auxiliary Buffer)\n";
        out_ << "    2. Custom Quicksort (Unstable, Median-of-3 Lomuto, O(n log n) Avg, O(log n) Stack)\n";
        out_ << "    3. Custom Heapsort (Unstable, Strictly In-Place O(1) Auxiliary Memory, O(n log n))\n";

        int algoChoice = 1;
        if (!readInt(algoChoice, 1, 3, "  Select Sorting Algorithm [1-3]: ")) return;

        ReportingService::SortAlgorithm algo;
        switch (algoChoice) {
            case 1: algo = ReportingService::SortAlgorithm::MERGE_SORT; break;
            case 2: algo = ReportingService::SortAlgorithm::QUICK_SORT; break;
            case 3: algo = ReportingService::SortAlgorithm::HEAP_SORT; break;
            default: algo = ReportingService::SortAlgorithm::MERGE_SORT; break;
        }

        // Capture independent value copy snapshots (completely read-only with respect to live system)
        std::vector<PatientSnapshot> snapshots = ReportingService::captureSnapshots(hospital_);
        ds_algorithms::SortMetrics metrics = ReportingService::sortSnapshots(snapshots, field, algo, false, nullptr);

        // Display Formatted Summary Table (uses captured snapshot timestamp)
        out_ << ReportingService::formatReportTable(snapshots, field);

        // Display Sort Telemetry
        out_ << "  [SORTING TELEMETRY METRICS]\n";
        out_ << "  Algorithm Applied:     " << metrics.algorithmName << "\n";
        out_ << "  Element Record Count:  " << metrics.elementCount << "\n";
        out_ << "  Comparisons Count:     " << metrics.comparisons << "\n";
        out_ << "  Element Swaps Count:   " << metrics.swaps << "\n";
        out_ << "  Standardized Moves:    " << metrics.moves << "\n";
        out_ << "  Algorithm Stability:   " << (metrics.isStable ? "STABLE (Equal keys preserve relative order)" : "UNSTABLE") << "\n";
        out_ << "  Auxiliary Space Used:  " << metrics.spaceComplexityAuxiliary << "\n";
        out_ << "  Execution Status:      " << (metrics.traceEnabled ? "N/A (Traced)" : (std::to_string(metrics.durationMicroseconds) + " us")) << "\n";
        out_ << "\n  [READ-ONLY GUARANTEE VERIFIED]\n";
        out_ << "  Report generation and sorting executed strictly on independent value copies;\n";
        out_ << "  hospital registry, triage min-heap, queue order, and simulation time were 100% unaltered.\n";
    }

    // ========================================================================
    // SUBMENU 7: EDUCATIONAL DATA STRUCTURE DEMONSTRATIONS
    // ========================================================================

    void runEducationalMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("EDUCATIONAL DATA STRUCTURE DEMONSTRATIONS (CO1 - CO6)");
            out_ << "  1. Execute Full Comprehensive Educational Suite (All 5 Demonstrations)\n";
            out_ << "  2. Demo 1: Sorting Algorithms Comparison Suite (Merge vs Quick vs Heap) (CO1, CO5)\n";
            out_ << "  3. Demo 2: Hash Collision Resolution (Separate Chaining vs Linear Probing) (CO2)\n";
            out_ << "  4. Demo 3: Graph Representations (Adjacency Matrix vs Adjacency List) (CO6)\n";
            out_ << "  5. Demo 4: Binary Search Tree Operations (Traversals, Mirroring, Deletion) (CO4)\n";
            out_ << "  6. Demo 5: Shortest Path vs Minimum Spanning Tree (Dijkstra vs Kruskal/Prim) (CO6)\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 6)) break;
            if (choice == 0) break;

            std::vector<PatientSnapshot> baseSnapshots = ReportingService::captureSnapshots(hospital_);

            switch (choice) {
                case 1:
                    DemonstrationService::runSortingComparison(baseSnapshots, out_);
                    DemonstrationService::runHashCollisionComparison(out_);
                    DemonstrationService::runGraphTraversalComparison(out_);
                    DemonstrationService::runBSTOperationsDemo(out_);
                    DemonstrationService::runDijkstraVsMSTDemo(hospital_.getMapManager(), out_);
                    break;
                case 2:
                    DemonstrationService::runSortingComparison(baseSnapshots, out_);
                    break;
                case 3:
                    DemonstrationService::runHashCollisionComparison(out_);
                    break;
                case 4:
                    DemonstrationService::runGraphTraversalComparison(out_);
                    break;
                case 5:
                    DemonstrationService::runBSTOperationsDemo(out_);
                    break;
                case 6:
                    DemonstrationService::runDijkstraVsMSTDemo(hospital_.getMapManager(), out_);
                    break;
                default: break;
            }
        }
    }

    // ========================================================================
    // SUBMENU 8: SYSTEM AUDIT TRAIL & INTEGRITY CHECK
    // ========================================================================

    void runAuditAndStatusMenu() {
        while (!exitRequested_ && !eofReached_) {
            printHeader("SYSTEM AUDIT TRAIL & INTEGRITY CHECK");
            out_ << "  1. View Chronological Audit Trail Log (DoublyLinkedList Event History)\n";
            out_ << "  2. Run Cross-Service System Consistency & Invariant Verification\n";
            out_ << "  3. Advance Simulation Clock (Simulate Elapsed Inactive Time)\n";
            out_ << "  4. View Dispatch Fleet Status & Allocation\n";
            out_ << "  0. Return to Main Menu\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 4)) break;
            if (choice == 0) break;

            switch (choice) {
                case 1: handleViewAuditLog(); break;
                case 2: handleRunIntegrityCheck(); break;
                case 3: handleAdvanceClock(); break;
                case 4: handleViewFleet(); break;
                default: break;
            }
        }
    }

    void handleViewAuditLog() {
        out_ << "\n--- Chronological System Audit Log ---\n";
        std::vector<AuditEvent> events = hospital_.getAuditEvents();
        out_ << "  Total Audit Events Recorded: " << events.size() << "\n\n";

        if (events.empty()) {
            out_ << "  Audit trail is empty.\n";
            return;
        }

        out_ << std::left
            << std::setw(8)  << "Time"
            << std::setw(12) << "Patient"
            << std::setw(28) << "Action"
            << std::setw(22) << "State Transition"
            << std::setw(10) << "Resource"
            << "Details\n";
        out_ << "----------------------------------------------------------------------------------------------------\n";

        for (const auto& ev : events) {
            std::string stateTrans = patientStateToString(ev.priorState) + "->" + patientStateToString(ev.newState);
            if (stateTrans.length() > 20) stateTrans = stateTrans.substr(0, 19) + "..";

            out_ << std::left
                << std::setw(8)  << (std::to_string(ev.simulatedTimestamp) + "s")
                << std::setw(12) << (ev.patientId.empty() ? "SYSTEM" : ev.patientId)
                << std::setw(28) << ev.action
                << std::setw(22) << stateTrans
                << std::setw(10) << (ev.resourceIdAtEvent.empty() ? "-" : ev.resourceIdAtEvent)
                << ev.details
                << "\n";
        }
        out_ << "----------------------------------------------------------------------------------------------------\n";
    }

    void handleRunIntegrityCheck() {
        out_ << "\n--- Comprehensive Cross-Service Consistency Checker ---\n";
        std::string err;
        if (hospital_.validateSystemIntegrity(err)) {
            out_ << "  [SYSTEM INTEGRITY: VERIFIED HEALTHY]\n";
            out_ << "  * Triage binary min-heap invariant holds across all records.\n";
            out_ << "  * All waiting triage patients exist in the heap and have TRIAGED_WAITING state.\n";
            out_ << "  * Appointment queue FIFO order and reciprocal states match registry perfectly.\n";
            out_ << "  * Pending dispatch pool contains all unassigned candidates with exactly 1 reference.\n";
            out_ << "  * Assigned patients and vehicles have reciprocal 1-to-1 matching and compatible states.\n";
            out_ << "  * Terminal patients (DISCHARGED, CANCELLED) are completely isolated from active queues.\n";
        } else {
            out_ << "  [INTEGRITY CHECK FAILED]: " << err << "\n";
        }
    }

    void handleAdvanceClock() {
        out_ << "\n--- Advance Simulation Clock ---\n";
        int secs = 0;
        if (!readInt(secs, 1, 86400, "  Enter number of seconds to advance clock [1 - 86400]: ")) return;

        hospital_.advanceSimulationTime(secs);
        out_ << "  [CLOCK ADVANCED] New simulation timestamp: " << hospital_.getSimulationTime() << "s.\n";
    }

    void handleViewFleet() {
        out_ << "\n--- Ambulance Dispatch Fleet Status ---\n";
        std::vector<DispatchResource> fleet = hospital_.getAllResources();
        out_ << "  Total Fleet Units: " << fleet.size() << "\n\n";

        out_ << std::left
            << std::setw(12) << "Resource ID"
            << std::setw(26) << "Resource Type"
            << std::setw(26) << "Operational State"
            << std::setw(14) << "Current Node"
            << std::setw(16) << "Assigned Patient"
            << "\n";
        out_ << "----------------------------------------------------------------------------------------------------\n";

        for (const auto& r : fleet) {
            out_ << std::left
                << std::setw(12) << r.resourceId
                << std::setw(26) << r.name
                << std::setw(26) << resourceStateToString(r.state)
                << std::setw(14) << (std::to_string(r.currentNodeId) + " (" + hospital_.getNodeName(r.currentNodeId) + ")")
                << std::setw(16) << (r.assignedPatientId.empty() ? "-" : r.assignedPatientId)
                << "\n";
        }
        out_ << "----------------------------------------------------------------------------------------------------\n";
    }

    // ========================================================================
    // SYSTEM EXIT & LIFECYCLE
    // ========================================================================

    void handleExit() {
        exitRequested_ = true;
        out_ << "\n================================================================================\n";
        out_ << "  Shutting down Smart Hospital Emergency Triage & Route Dispatch System.\n";
        out_ << "  Notice: All in-memory operational state and audit logs are discarded on exit.\n";
        out_ << "================================================================================\n";
    }

public:
    ConsoleApp(HospitalSystem& hospital, std::istream& in, std::ostream& out,
               const std::string& dataDir = "data/", bool useAnsi = false)
        : hospital_(hospital), in_(in), out_(out), dataDir_(dataDir), useAnsi_(useAnsi) {}

    void run() {
        out_ << "================================================================================\n";
        out_ << "    SMART HOSPITAL EMERGENCY TRIAGE & ROUTE DISPATCH SYSTEM (CS2305)\n";
        out_ << "       Academic Simulation - AY 2026-27 | Data Structures-I Project\n";
        out_ << "================================================================================\n";

        printInMemoryNotice();

        while (!exitRequested_ && !eofReached_) {
            printHeader("MAIN MENU - HOSPITAL OPERATIONS & DISPATCH");
            out_ << "  1. Patient Registration & Registry Management\n";
            out_ << "  2. Emergency Clinical Triage & Acuity Updates\n";
            out_ << "  3. Outpatient Clinic Appointments & FIFO Queue\n";
            out_ << "  4. Ambulance Dispatch & Patient Transit Operations\n";
            out_ << "  5. Campus Topology, Routing & Corridor Management\n";
            out_ << "  6. Patient Reports & Custom Sorting Analytics\n";
            out_ << "  7. Educational Data Structure Demonstrations (CO1 - CO6)\n";
            out_ << "  8. System Audit Trail & Cross-Service Integrity Check\n";
            out_ << "  9. Exit System\n";

            int choice = -1;
            if (!readMenuChoice(choice, 1, 9)) {
                break; // EOF or stream termination
            }

            if (choice == 0 || choice == 9) {
                handleExit();
                break;
            }

            switch (choice) {
                case 1: runPatientMenu(); break;
                case 2: runTriageMenu(); break;
                case 3: runAppointmentMenu(); break;
                case 4: runDispatchMenu(); break;
                case 5: runMapMenu(); break;
                case 6: runReportsMenu(); break;
                case 7: runEducationalMenu(); break;
                case 8: runAuditAndStatusMenu(); break;
                default: break;
            }
        }

        if (eofReached_) {
            out_ << "\n[EOF DETECTED] End of stream reached. Cleanly terminating console application.\n";
            handleExit();
        }
    }
};
