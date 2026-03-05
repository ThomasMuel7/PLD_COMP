#!/usr/bin/env python3
import subprocess
import platform

# Définition des codes couleurs ANSI
GREEN = "\033[92m"
ORANGE = "\033[93m"
RED = "\033[91m"
BOLD = "\033[1m"
RESET = "\033[0m"

TEST_SCRIPT = '../ifcc-test.py'
TEST_FILES_DIR = '../testfiles'

def run_tests():
    print(f"Lancement des tests de {TEST_FILES_DIR}")

    # Détection de l'OS
    cmd = ['python3', TEST_SCRIPT, TEST_FILES_DIR]

    if platform.system() == "Darwin":  # macOS
        cmd = ["arch", "-x86_64"] + cmd

    # Exécution du script de test
    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output = result.stdout
    lines = [line.strip() for line in output.splitlines() if line.strip()]

    total_tests = 0
    passed_tests = 0
    failed_tests = []

    # Analyse de la sortie
    for i in range(len(lines)):
        if lines[i].startswith("TEST-CASE:"):
            total_tests += 1
            test_name = lines[i].replace("TEST-CASE:", "").strip()

            if i + 1 < len(lines):
                result_line = lines[i+1]
                if "TEST OK" in result_line:
                    passed_tests += 1
                elif "TEST FAIL" in result_line:
                    failed_tests.append(test_name)

    # --- Affichage des résultats ---
    print("\n" + "="*35)
    print(f"{BOLD}       RAPPORT DE TEST{RESET}")
    print("="*35)

    if total_tests > 0:
        pass_percentage = (passed_tests / total_tests) * 100

        if pass_percentage > 80:
            rate_color = GREEN
        elif pass_percentage > 50:
            rate_color = ORANGE
        else:
            rate_color = RED

        print(f"Total tests    : {total_tests}")
        print(f"Succès         : {GREEN}{passed_tests}{RESET}")
        print(f"Échecs         : {RED}{len(failed_tests)}{RESET}")
        print(f"Taux de succès : {rate_color}{pass_percentage:.2f}%{RESET}")

        if failed_tests:
            print(f"\n{RED}{BOLD}Détail des tests échoués :{RESET}")
            for test in failed_tests:
                print(f" {RED}- {test}{RESET}")
    else:
        print(f"{RED}Aucun test détecté. Vérifie tes chemins.{RESET}")


if __name__ == "__main__":
    run_tests()