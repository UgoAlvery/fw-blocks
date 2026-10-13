# Controles de specification.
#
# Ils tournent sans code metier, ce qui donne a la CI quelque chose de reel a
# verifier des le stade v0.0-spec : la documentation reste coherente et les
# vecteurs restent ceux que produit l'oracle. Voir docs/test-plan.md.

find_package(Python3 COMPONENTS Interpreter QUIET)

if(NOT Python3_Interpreter_FOUND)
    message(WARNING
        "Python 3 introuvable : les controles de specification sont desactives. "
        "Ils restent executes en CI.")
    return()
endif()

# L'option -I isole l'interpreteur : ni repertoire courant dans sys.path, ni
# variables d'environnement PYTHON*. Les scripts n'importent que la
# bibliotheque standard, donc rien a perdre, et un fichier depose dans
# l'arborescence ne peut pas detourner un import.

add_test(NAME spec_vectors_up_to_date
         COMMAND ${Python3_EXECUTABLE} -I
                 "${CMAKE_SOURCE_DIR}/tools/gen_vectors.py" --check)
set_tests_properties(spec_vectors_up_to_date PROPERTIES
    LABELS "spec" TIMEOUT 60)

add_test(NAME spec_docs_consistent
         COMMAND ${Python3_EXECUTABLE} -I
                 "${CMAKE_SOURCE_DIR}/tools/check_docs.py")
set_tests_properties(spec_docs_consistent PROPERTIES
    LABELS "spec" TIMEOUT 60)

message(STATUS "Controles de specification : 2 tests enregistres")
