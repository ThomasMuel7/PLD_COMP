#!/usr/bin/env python3

import subprocess
import platform
import os
import sys

# Définition des codes couleurs ANSI
GREEN  = "\033[92m"
ORANGE = "\033[93m"
RED    = "\033[91m"
BOLD   = "\033[1m"
RESET  = "\033[0m"

TEST_SCRIPT    = "../ifcc-test.py"
TEST_FILES_DIR = "../testfiles"
COMPILER_DIR   = "."   # répertoire où se trouve l'exécutable ifcc

def detect_target():
    """Détecte la cible via la commande 'arch' si disponible, sinon fallback sur platform."""
    try:
        result = subprocess.run(["arch"], capture_output=True, text=True)
        arch = result.stdout.strip()
        if arch == "arm64":
            return "arm"
        elif arch in ["i386", "x86_64"]:
            return "x86"
        else:
            return "x86"
    except FileNotFoundError:
        # 'arch' non disponible (Linux, etc.), fallback sur platform
        if platform.system() == "Darwin" and platform.machine() == "arm64":
            return "arm"
        return "x86"

def create_ifcc_wrapper(target):
    """
    Crée un script shell qui injecte '-target <target>' avant de déléguer
    au vrai compilateur renommé 'ifcc_real'.

    ifcc-test.py appelle '{compiler_dir}/ifcc input.c' sans possibilité
    d'y ajouter des arguments — ce wrapper est totalement transparent.
    """
    real_ifcc    = os.path.abspath(os.path.join(COMPILER_DIR, "ifcc_real"))
    wrapper_path = os.path.abspath(os.path.join(COMPILER_DIR, "ifcc_wrapper.sh"))
    with open(wrapper_path, "w") as f:
        f.write("#!/bin/bash\n")
        f.write(f'exec "{real_ifcc}" -target {target} "$@"\n')
    os.chmod(wrapper_path, 0o755)
    return wrapper_path

def install_wrapper(wrapper_path):
    """Renomme ifcc → ifcc_real puis le wrapper → ifcc."""
    ifcc_path      = os.path.join(COMPILER_DIR, "ifcc")
    ifcc_real_path = os.path.join(COMPILER_DIR, "ifcc_real")
    os.rename(ifcc_path, ifcc_real_path)
    os.rename(wrapper_path, ifcc_path)

def uninstall_wrapper():
    """Restaure ifcc_real → ifcc et supprime le wrapper."""
    ifcc_path      = os.path.join(COMPILER_DIR, "ifcc")
    ifcc_real_path = os.path.join(COMPILER_DIR, "ifcc_real")
    if os.path.exists(ifcc_path):
        os.remove(ifcc_path)
    if os.path.exists(ifcc_real_path):
        os.rename(ifcc_real_path, ifcc_path)

def run_tests():
    target = detect_target()
    print(f"Architecture détectée : {BOLD}{target.upper()}{RESET}")
    print(f"Lancement des tests depuis {TEST_FILES_DIR}\n")

    # Vérifier que ifcc est compilé
    ifcc_path = os.path.join(COMPILER_DIR, "ifcc")
    if not os.path.exists(ifcc_path):
        print(f"{RED}Erreur : l'exécutable 'ifcc' est introuvable dans '{COMPILER_DIR}'.{RESET}")
        print("Lance 'make' d'abord.")
        sys.exit(1)

    # Installer le wrapper
    wrapper_path = create_ifcc_wrapper(target)
    install_wrapper(wrapper_path)

    try:
        result = subprocess.run(
            ["python3", TEST_SCRIPT, TEST_FILES_DIR],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        output = result.stdout
    finally:
        # Toujours restaurer le vrai ifcc, même en cas d'exception
        uninstall_wrapper()

    lines = [line.strip() for line in output.splitlines() if line.strip()]

    total_tests  = 0
    passed_tests = 0
    failed_tests = []

    for i in range(len(lines)):
        if lines[i].startswith("TEST-CASE:"):
            total_tests += 1
            test_name = lines[i].replace("TEST-CASE:", "").strip()
            if i + 1 < len(lines):
                result_line = lines[i + 1]
                if "TEST OK" in result_line:
                    passed_tests += 1
                elif "TEST FAIL" in result_line:
                    failed_tests.append((test_name, result_line))

    # --- Affichage ---
    print("=" * 40)
    print(f"{BOLD}         RAPPORT DE TEST{RESET}")
    print(f"         target : {BOLD}{target.upper()}{RESET}")
    print("=" * 40)

    if total_tests > 0:
        pass_percentage = (passed_tests / total_tests) * 100
        rate_color = GREEN if pass_percentage > 80 else (ORANGE if pass_percentage > 50 else RED)

        print(f"Total tests    : {total_tests}")
        print(f"Succès         : {GREEN}{passed_tests}{RESET}")
        print(f"Échecs         : {RED}{len(failed_tests)}{RESET}")
        print(f"Taux de succès : {rate_color}{pass_percentage:.2f}%{RESET}")

        if failed_tests:
            print(f"\n{RED}{BOLD}Détail des tests échoués :{RESET}")
            for test_name, reason in failed_tests:
                detail = ""
                if "(" in reason and ")" in reason:
                    detail = f" {ORANGE}({reason[reason.find('(')+1:reason.find(')')]}){RESET}"
                print(f"  {RED}✗{RESET} {test_name}{detail}")
    else:
        print(f"{RED}Aucun test détecté. Vérifie tes chemins.{RESET}")

if __name__ == "__main__":
    run_tests()