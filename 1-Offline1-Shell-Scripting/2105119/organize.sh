#!/bin/bash

execute_and_match() {
    local studentDir="$1"
    local lang="$2"
    local testsDir="$3"
    local answersDir="$4"
    local matched=0
    local not_matched=0

    case "$lang" in
        "C")
            gcc "$studentDir/main.c" -o "$studentDir/main.out"
            ;;
        "C++")
            g++ "$studentDir/main.cpp" -o "$studentDir/main.out" 
            ;;
        "Java")
            javac "$studentDir/Main.java"
            ;;
    esac


    for testFile in "$testsDir"/test*.txt; do

        testNum="${testFile##*/}"
        testNum="${testNum%.*}"
        testNum="${testNum: -1}"
        outFile="$studentDir/out$testNum.txt"
        ansFile="$answersDir/ans$testNum.txt"

        case "$lang" in
            "Python")
                python3 "$studentDir/main.py" < "$testFile" > "$outFile"
                ;;
            "C"|"C++")
                "$studentDir/main.out" < "$testFile" > "$outFile"
                ;;
            "Java")
                java -cp "$studentDir" Main < "$testFile" > "$outFile"
                ;;
        esac

        if diff -q "$outFile" "$ansFile" >/dev/null; then
            ((matched++))
        else
            ((not_matched++))
        fi
    done

    echo "$matched,$not_matched"
}






analyze_code() {
    local file="$1"
    local lang="$2"
    local lineCount=0
    local commentCount=0
    local functionCount=0


    if [[ ! " $@ " =~ " -nolc " ]]; then
        lineCount=$(wc -l < "$file" | xargs)
    fi

    if [[ ! " $@ " =~ " -nocc " ]]; then
        if [[ "$lang" == "Python" ]]; then
            commentCount=$(grep -c '[[:space:]]*#' "$file")
        else
            commentCount=$(grep -c '[[:space:]]*//' "$file")
        fi
    fi

    if [[ ! " $@ " =~ " -nofc " ]]; then
        if [[ "$lang" == "Python" ]]; then
            patternPy="[[:space:]]*def[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(.*\):"
            functionCount=$(grep -E "$patternPy" "$file" | wc -l)
        else
            pattern="[[:space:]]*([a-zA-Z_][a-zA-Z0-9_]*[[:space:]]+\**[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\([^;]*\)[[:space:]]*\{)"
            functionCount=$(grep -E "$pattern" "$file" | wc -l)
        fi
    fi

    echo "$lineCount,$commentCount,$functionCount"
}

if [ $# -lt 4 ]; then
    echo "Usage: $0 <submissions_dir> <target_dir> <tests_dir> <answers_dir> [OPTIONS]"
    exit 1
fi




SUBMISSIONS_DIR="$1"
TARGET_DIR="$2"
TESTS_DIR="$3"
ANSWERS_DIR="$4"



csv_row_name="student_id,student_name,language"
if [[ ! "$@" =~ "-noexecute" ]]; then
    csv_row_name="$csv_row_name,matched,not_matched"
fi
if [[ ! "$@" =~ "-nolc" ]]; then
    csv_row_name="$csv_row_name,line_count"
fi
if [[ ! "$@" =~ "-nocc" ]]; then
    csv_row_name="$csv_row_name,comment_count"
fi
if [[ ! "$@" =~ "-nofc" ]]; then
    csv_row_name="$csv_row_name,function_count"
fi


mkdir -p "$TARGET_DIR"
echo "$csv_row_name" > "$TARGET_DIR/result.csv"


for zip_file in "$SUBMISSIONS_DIR"/*.zip; do
    fileNameWithoutZip="${zip_file%.zip}"

    tempName="${fileNameWithoutZip%%_*}"
    studentName="${tempName##*/}"
    studentName="\"${studentName}\""
    studentId="${fileNameWithoutZip##*_}"

    if [[ "$@" =~ "-v" ]]; then
        echo "Organizing files of $studentId"
    fi
    
    tempDir=$(mktemp -d)
    unzip -q "$zip_file" -d "$tempDir"


    # Find code file with safety checks
    codeFileType=$(find "$tempDir" -type f \( -name "*.java" -o -name "*.py" -o -name "*.cpp" -o -name "*.c" \))
    if [ -z "$codeFileType" ]; then
        echo "No valid code file found for student ID: $studentId"
        continue
    fi

    # Determine language and target filename
    case "$codeFileType" in
        *.java)
            lang="Java"
            name="Main.java"
            ;;
        *.py)
            lang="Python" 
            name="main.py"
            ;;
        *.cpp)
            lang="C++"
            name="main.cpp"
            ;;
        *.c)
            lang="C"
            name="main.c"
            ;;
    esac

    studentDir="$TARGET_DIR/$lang/$studentId"
    mkdir -p "$studentDir"
    cp "$codeFileType" "$studentDir/$name"


    # Analyze code metrics
    # Task B
    analysisResult=$(analyze_code "$studentDir/$name" "$lang")
    IFS=',' read -r lineCount commentCount functionCount <<< "$analysisResult"
    # echo " line comment function $lineCount $commentCount $functionCount"


    csv_row_value="$studentId,$studentName,$lang"

    if [[ "$@" =~ "-noexecute" ]]; then
        if [[ ! "$@" =~ "-nolc" ]]; then
        csv_row_value="$csv_row_value,$lineCount"
        fi
        if [[ ! "$@" =~ "-nocc" ]]; then
            csv_row_value="$csv_row_value,$commentCount"
        fi
        if [[ ! "$@" =~ "-nofc" ]]; then
            csv_row_value="$csv_row_value,$functionCount"
        fi
        # echo "debug $csv_row_value"
        # echo "$csv_row_value"
        echo "$csv_row_value" >> "$TARGET_DIR/result.csv"

    else
        if [[ "$@" =~ "-v" ]]; then
            echo "Executing files of $studentId"
        fi
        result=$(execute_and_match "$studentDir" "$lang" "$TESTS_DIR" "$ANSWERS_DIR")
        IFS=',' read -r matched not_matched <<< "$result"

        csv_row_value="$csv_row_value,$matched,$not_matched"
        if [[ ! "$@" =~ "-nolc" ]]; then
            csv_row_value="$csv_row_value,$lineCount"
        fi
        if [[ ! "$@" =~ "-nocc" ]]; then
            csv_row_value="$csv_row_value,$commentCount"
        fi
        if [[ ! "$@" =~ "-nofc" ]]; then
            csv_row_value="$csv_row_value,$functionCount"
        fi
        echo "$csv_row_value" >> "$TARGET_DIR/result.csv"
    fi

    
    # Cleanup
    rm -rf "$tempDir"

done