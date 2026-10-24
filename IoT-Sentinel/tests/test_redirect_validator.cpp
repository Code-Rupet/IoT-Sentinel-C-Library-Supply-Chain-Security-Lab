/**
 * C++ Host Test: Redirect Safety Validator & Test Event Conformance
 * =================================================================
 * Compiles and runs under native g++ to test:
 *  - Safe Google Redirect domain extraction & filtering
 *  - Protection against attacker spoofed subdomains
 */

#include <iostream>
#include <string>
#include <cassert>
#include <algorithm>

// Standalone C++ implementation of isSafeGoogleRedirect logic from GoogleDriveClient.cpp
bool isSafeGoogleRedirect(const std::string& url) {
    if (url.rfind("https://", 0) != 0) {
        std::cout << "  [REJECTED] Non-HTTPS redirect: " << url << std::endl;
        return false;
    }

    size_t hostStart = 8; // Length of "https://"
    size_t slashIdx = url.find('/', hostStart);
    std::string host = (slashIdx == std::string::npos) ? url.substr(hostStart) : url.substr(hostStart, slashIdx - hostStart);
    
    size_t colonIdx = host.find(':');
    if (colonIdx != std::string::npos) {
        host = host.substr(0, colonIdx);
    }
    
    std::transform(host.begin(), host.end(), host.begin(), ::tolower);

    // Exact matches
    if (host == "script.google.com" || host == "script.googleusercontent.com") {
        return true;
    }

    // Subdomain matches: must end with .google.com or .googleusercontent.com
    const std::string suffixGoogle = ".google.com";
    const std::string suffixCDN = ".googleusercontent.com";

    if (host.length() > suffixGoogle.length() &&
        host.compare(host.length() - suffixGoogle.length(), suffixGoogle.length(), suffixGoogle) == 0) {
        return true;
    }

    if (host.length() > suffixCDN.length() &&
        host.compare(host.length() - suffixCDN.length(), suffixCDN.length(), suffixCDN) == 0) {
        return true;
    }

    std::cout << "  [REJECTED] Disallowed host: " << host << std::endl;
    return false;
}

int main() {
    std::cout << "===================================================================" << std::endl;
    std::cout << "   IoT Sentinel — C++ Redirect Validation Unit Tests              " << std::endl;
    std::cout << "===================================================================" << std::endl;

    int passed = 0;
    int total = 0;

    auto test = [&](const std::string& name, bool condition) {
        total++;
        if (condition) {
            std::cout << "[PASS] Test " << total << ": " << name << std::endl;
            passed++;
        } else {
            std::cerr << "[FAIL] Test " << total << ": " << name << std::endl;
        }
    };

    // Valid Google URLs
    test("Accept official script.google.com", 
         isSafeGoogleRedirect("https://script.google.com/macros/s/123/exec"));
    test("Accept official script.googleusercontent.com", 
         isSafeGoogleRedirect("https://script.googleusercontent.com/macros/echo?user_content_key=xyz"));
    test("Accept google.com subdomains", 
         isSafeGoogleRedirect("https://sub.script.google.com/path"));

    // Attacks & Spoofing attempts
    test("Reject HTTP (plaintext)", 
         !isSafeGoogleRedirect("http://script.google.com/macros/s/123/exec"));
    test("Reject attacker suffix spoofing (google.com.attacker.com)", 
         !isSafeGoogleRedirect("https://google.com.attacker.com/macros/echo"));
    test("Reject attacker prefix spoofing (attacker-google.com)", 
         !isSafeGoogleRedirect("https://attacker-google.com/macros/echo"));
    test("Reject CDN suffix spoofing (script.googleusercontent.com.evil.com)", 
         !isSafeGoogleRedirect("https://script.googleusercontent.com.evil.com/path"));
    test("Reject completely arbitrary external domain", 
         !isSafeGoogleRedirect("https://evil-server.org/harvest?token=123"));
    test("Reject relative URL without https", 
         !isSafeGoogleRedirect("/local/redirect/path"));

    std::cout << "\n-------------------------------------------------------------------" << std::endl;
    std::cout << " Results: " << passed << " / " << total << " passed" << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;

    return (passed == total) ? 0 : 1;
}
