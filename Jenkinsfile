void setBuildStatus(String message, String state) {
  step([
      $class: "GitHubCommitStatusSetter",
      reposSource: [$class: "ManuallyEnteredRepositorySource", url: "https://github.com/xiantongw/srd"],
      contextSource: [$class: "ManuallyEnteredCommitContextSource", context: "ci/jenkins/build-status"],
      errorHandlers: [[$class: "ChangingBuildStatusErrorHandler", result: "UNSTABLE"]],
      statusResultSource: [ $class: "ConditionalStatusResultSource", 
        results: [[$class: "AnyBuildResult", message: message, state: state]] ]
  ]);
}

pipeline {
    agent {
        docker {
            image 'ubuntu:22.04'
            args '-u root:root'
        }
    }

    options {
        skipDefaultCheckout(true)
        timestamps()
    }
    
    stages {
        stage('Checkout') {
            steps {
                checkout scm
                script {
                    setBuildStatus("Build started", "PENDING")
                }
            }
        }
        stage('Install dependencies') {
            steps {
                sh '''
                    apt-get update
                    apt-get install -y curl build-essential git python3 clang-tidy
                    curl -L https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 -o /usr/local/bin/bazelisk
                    chmod +x /usr/local/bin/bazelisk
                '''
            }
        }
        stage('Lint') {
            steps {
                sh '''
                    set -eux
                    bazelisk --enable_bzlmod run @hedron_compile_commands//:refresh_all
                    FILES="$(git ls-files '*.c' '*.cc' '*.cpp' '*.cxx')"
                    [ -z "$FILES" ] && echo "No C/C++ sources to lint." && exit 0

                    GCC_VER=13
                    TRIPLE="$(gcc -print-multiarch)"
                    GCC_TOOLCHAIN="/usr/lib/gcc/${TRIPLE}/${GCC_VER}"

                    echo "$FILES" | xargs -r -n 32 clang-tidy -p . --warnings-as-errors='*' \
                      --extra-arg-before=--driver-mode=g++ \
                      --extra-arg-before="--gcc-toolchain=${GCC_TOOLCHAIN}" \
                      --extra-arg="-isystem/usr/include/c++/${GCC_VER}" \
                      --extra-arg="-isystem/usr/include/${TRIPLE}/c++/${GCC_VER}" \
                      --extra-arg="-isystem${GCC_TOOLCHAIN}/include"
                '''
            }
        }
        stage('Build') {
            steps {
                sh 'bazelisk build //...'
            }
        }
        stage('Test') {
            steps {
                sh 'bazelisk test //tests/... --test_output=all'
            }
        }
    }
    post {
        success {
             script { setBuildStatus("Build succeeded", "SUCCESS") };
        }
        failure {
             script { setBuildStatus("Build failed", "FAILURE") };
        }
    }
}