#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "models/Patient.hpp"
#include "models/PatientSnapshot.hpp"
#include "algorithms/CustomSorting.hpp"
#include "services/HospitalSystem.hpp"

/**
 * @brief Reporting Service for generating independent value-based patient reports
 *        and sorting comparisons (CS2305 Course Outcome CO1, with Heapsort supporting CO5).
 * 
 * NON-MUTATING INVARIANT:
 * - Generates independent value copy PatientSnapshot records purely from const inspection.
 * - Leaves registry records, triage min-heap order, appointment queue, dispatch fleet,
 *   simulation clock, and audit history 100% unaltered.
 */
class ReportingService {
public:
    enum class SortAlgorithm {
        MERGE_SORT,
        QUICK_SORT,
        HEAP_SORT
    };

    enum class SortField {
        AGE,
        ARRIVAL_TIME,
        URGENCY,
        BILLING_AMOUNT,
        WAITING_DURATION
    };

    // ========================================================================
    // DETERMINISTIC COMPARATORS (WITH DOCUMENTED TIE-BREAKING)
    // ========================================================================

    struct CompareByAge {
        bool operator()(const PatientSnapshot& a, const PatientSnapshot& b) const noexcept {
            if (a.age != b.age) {
                return a.age < b.age;
            }
            if (a.arrivalSequence != b.arrivalSequence) {
                return a.arrivalSequence < b.arrivalSequence;
            }
            return a.patientId < b.patientId;
        }
    };

    struct CompareByArrivalTime {
        bool operator()(const PatientSnapshot& a, const PatientSnapshot& b) const noexcept {
            if (a.simulatedArrivalTime != b.simulatedArrivalTime) {
                return a.simulatedArrivalTime < b.simulatedArrivalTime;
            }
            if (a.arrivalSequence != b.arrivalSequence) {
                return a.arrivalSequence < b.arrivalSequence;
            }
            return a.patientId < b.patientId;
        }
    };

    struct CompareByUrgency {
        bool operator()(const PatientSnapshot& a, const PatientSnapshot& b) const noexcept {
            int ua = urgencyToInt(a.urgency);
            int ub = urgencyToInt(b.urgency);
            if (ua != ub) {
                return ua < ub; // Highest acuity (1 - Resuscitation) first
            }
            if (a.arrivalSequence != b.arrivalSequence) {
                return a.arrivalSequence < b.arrivalSequence; // FIFO tie-breaker
            }
            return a.patientId < b.patientId;
        }
    };

    // Pure urgency comparator without tie-breaking, used to test algorithm stability
    struct CompareByUrgencyKeyOnly {
        bool operator()(const PatientSnapshot& a, const PatientSnapshot& b) const noexcept {
            return urgencyToInt(a.urgency) < urgencyToInt(b.urgency);
        }
    };

    struct CompareByBillingAmount {
        bool operator()(const PatientSnapshot& a, const PatientSnapshot& b) const noexcept {
            if (a.billingAmount != b.billingAmount) {
                return a.billingAmount < b.billingAmount;
            }
            if (a.arrivalSequence != b.arrivalSequence) {
                return a.arrivalSequence < b.arrivalSequence;
            }
            return a.patientId < b.patientId;
        }
    };

    struct CompareByWaitingDuration {
        bool operator()(const PatientSnapshot& a, const PatientSnapshot& b) const noexcept {
            // Actively waiting patients come before non-waiting/historical patients
            if (a.isWaitingDurationApplicable != b.isWaitingDurationApplicable) {
                return a.isWaitingDurationApplicable > b.isWaitingDurationApplicable;
            }
            // For actively waiting patients, longest wait time comes first (descending)
            if (a.isWaitingDurationApplicable && b.isWaitingDurationApplicable) {
                if (a.waitingDurationSeconds != b.waitingDurationSeconds) {
                    return a.waitingDurationSeconds > b.waitingDurationSeconds;
                }
            }
            // FIFO tie-breaker for identical waiting durations or non-waiting records
            if (a.arrivalSequence != b.arrivalSequence) {
                return a.arrivalSequence < b.arrivalSequence;
            }
            return a.patientId < b.patientId;
        }
    };

    // ========================================================================
    // SNAPSHOT EXTRACTION
    // ========================================================================

    /**
     * @brief Extracts value-based PatientSnapshot records from the hospital system.
     *        Guaranteed read-only and non-mutating.
     */
    static std::vector<PatientSnapshot> captureSnapshots(const HospitalSystem& sys) {
        std::vector<PatientSnapshot> snapshots;
        int64_t timestamp = sys.getSimulationTime();
        const auto& allPatients = sys.getRegistry().getAllPatients();
        snapshots.reserve(allPatients.size());

        for (const Patient* p : allPatients) {
            if (p != nullptr) {
                snapshots.emplace_back(*p, timestamp);
            }
        }
        return snapshots;
    }

    // ========================================================================
    // SORTING DISPATCH
    // ========================================================================

    template <typename Comparator>
    static void executeSort(std::vector<PatientSnapshot>& data, SortAlgorithm algo,
                            Comparator comp, ds_algorithms::SortMetrics& metrics,
                            bool enableTrace = false, std::vector<std::string>* traceLog = nullptr) {
        switch (algo) {
            case SortAlgorithm::MERGE_SORT:
                ds_algorithms::customMergeSort(data, comp, metrics, enableTrace, traceLog);
                break;
            case SortAlgorithm::QUICK_SORT:
                ds_algorithms::customQuickSort(data, comp, metrics, enableTrace, traceLog);
                break;
            case SortAlgorithm::HEAP_SORT:
                ds_algorithms::customHeapSort(data, comp, metrics, enableTrace, traceLog);
                break;
        }
    }

    /**
     * @brief Sorts snapshots by requested field and algorithm, returning sort telemetry.
     */
    static ds_algorithms::SortMetrics sortSnapshots(std::vector<PatientSnapshot>& data,
                                                    SortField field,
                                                    SortAlgorithm algo = SortAlgorithm::MERGE_SORT,
                                                    bool enableTrace = false,
                                                    std::vector<std::string>* traceLog = nullptr) {
        ds_algorithms::SortMetrics metrics;
        switch (field) {
            case SortField::AGE:
                executeSort(data, algo, CompareByAge(), metrics, enableTrace, traceLog);
                break;
            case SortField::ARRIVAL_TIME:
                executeSort(data, algo, CompareByArrivalTime(), metrics, enableTrace, traceLog);
                break;
            case SortField::URGENCY:
                executeSort(data, algo, CompareByUrgency(), metrics, enableTrace, traceLog);
                break;
            case SortField::BILLING_AMOUNT:
                executeSort(data, algo, CompareByBillingAmount(), metrics, enableTrace, traceLog);
                break;
            case SortField::WAITING_DURATION:
                executeSort(data, algo, CompareByWaitingDuration(), metrics, enableTrace, traceLog);
                break;
        }
        return metrics;
    }

    // ========================================================================
    // REPORT FORMATTING
    // ========================================================================

    static std::string fieldToString(SortField field) {
        switch (field) {
            case SortField::AGE: return "Age (Ascending)";
            case SortField::ARRIVAL_TIME: return "Simulated Arrival Time (Ascending)";
            case SortField::URGENCY: return "Clinical Urgency (Acuity 1 to 5)";
            case SortField::BILLING_AMOUNT: return "Billing Amount ($ Ascending)";
            case SortField::WAITING_DURATION: return "Active Waiting Duration (Longest First)";
        }
        return "Unknown";
    }

    static std::string algorithmToString(SortAlgorithm algo) {
        switch (algo) {
            case SortAlgorithm::MERGE_SORT: return "Custom Merge Sort (Stable, O(n log n))";
            case SortAlgorithm::QUICK_SORT: return "Custom Quicksort (Median-of-3, Lomuto, O(n log n) avg)";
            case SortAlgorithm::HEAP_SORT: return "Custom Heapsort (In-Place, O(1) space, O(n log n))";
        }
        return "Unknown";
    }

    static std::string formatReportTable(const std::vector<PatientSnapshot>& data,
                                         SortField sortedBy,
                                         int64_t fallbackTimestamp = 0) {
        int64_t displayTimestamp = data.empty() ? fallbackTimestamp : data.front().reportTimestamp;
        std::ostringstream oss;
        oss << "=======================================================================================================================================================\n";
        oss << "                                                       HOSPITAL PATIENT SUMMARY REPORT SNAPSHOT\n";
        oss << "    Report Snapshot Captured: " << displayTimestamp << "s | Sorted By: " << fieldToString(sortedBy) << "\n";
        oss << "    Total Patient Records:    " << data.size() << " | Note: Use Patient Details for unabbreviated names (up to 64 chars) and complaints (up to 128 chars)\n";
        oss << "=======================================================================================================================================================\n";
        oss << std::left
            << std::setw(18) << "ID"
            << std::setw(22) << "Full Name"
            << std::setw(5)  << "Age"
            << std::setw(18) << "Urgency"
            << std::setw(24) << "State"
            << std::setw(8)  << "ArrSeq"
            << std::setw(14) << "ArrTime"
            << std::setw(28) << "Waiting Time"
            << std::setw(14) << "Billing"
            << "\n";
        oss << "-------------------------------------------------------------------------------------------------------------------------------------------------------\n";

        for (const auto& s : data) {
            std::string urgStr = std::to_string(urgencyToInt(s.urgency)) + "-" + urgencyToString(s.urgency);

            std::string nameStr = s.fullName;
            if (nameStr.length() > 20) nameStr = nameStr.substr(0, 18) + "..";

            std::ostringstream billOss;
            billOss << "$" << std::fixed << std::setprecision(2) << s.billingAmount;

            oss << std::left
                << std::setw(18) << s.patientId
                << std::setw(22) << nameStr
                << std::setw(5)  << s.age
                << std::setw(18) << urgStr
                << std::setw(24) << patientStateToString(s.state)
                << std::setw(8)  << s.arrivalSequence
                << std::setw(14) << (std::to_string(s.simulatedArrivalTime) + "s")
                << std::setw(28) << s.waitingDurationFormatted
                << std::setw(14) << billOss.str()
                << "\n";
        }
        oss << "=======================================================================================================================================================\n";
        return oss.str();
    }

    /**
     * @brief Formats a complete, unabbreviated patient dossier.
     *        Provides full names (up to 64 chars) and chief complaints (up to 128 chars)
     *        separately from the summary table to prevent column truncation/clipping.
     */
    static std::string formatPatientDetail(const PatientSnapshot& s, const CampusMapManager* map = nullptr) {
        std::ostringstream oss;
        oss << "--------------------------------------------------------------------------------\n";
        oss << "                          FULL PATIENT CLINICAL DOSSIER                         \n";
        oss << "--------------------------------------------------------------------------------\n";
        oss << "  Patient ID:          " << s.patientId << "\n";
        oss << "  Full Legal Name:     " << s.fullName << "\n";
        oss << "  Age:                 " << s.age << " years\n";
        oss << "  Chief Complaint:     " << s.complaint << "\n";
        oss << "  Clinical Urgency:    " << urgencyToInt(s.urgency) << " - " << urgencyToString(s.urgency) << "\n";
        oss << "  Lifecycle State:     " << patientStateToString(s.state) << "\n";
        oss << "  Arrival Sequence:    #" << s.arrivalSequence << "\n";
        oss << "  Simulated Arrival:   " << s.simulatedArrivalTime << "s (clock timestamp)\n";
        oss << "  Active Waiting:      ";
        if (s.isWaitingDurationApplicable) {
            oss << s.waitingDurationSeconds << " seconds (actively waiting in triage/queue/unassigned pending)\n";
        } else {
            oss << s.waitingDurationFormatted << " (historical/treatment phase; active wait unavailable)\n";
        }
        oss << "  Current Location:    Node " << s.currentLocationNode;
        if (map && map->isValidNode(s.currentLocationNode)) {
            oss << " (" << map->getNodeName(s.currentLocationNode) << ")";
        }
        oss << "\n";
        oss << "  Target Location:     Node " << s.targetLocationNode;
        if (map && map->isValidNode(s.targetLocationNode)) {
            oss << " (" << map->getNodeName(s.targetLocationNode) << ")";
        }
        oss << "\n";
        oss << "  Assigned Resource:   " << (s.assignedResourceId.empty() ? "None (Unassigned)" : s.assignedResourceId) << "\n";
        oss << "  Billing Balance:     $" << std::fixed << std::setprecision(2) << s.billingAmount << "\n";
        oss << "  Snapshot Timestamp:  " << s.reportTimestamp << "s\n";
        oss << "--------------------------------------------------------------------------------\n";
        return oss.str();
    }
};
