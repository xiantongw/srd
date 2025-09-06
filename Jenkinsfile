void setBuildStatus(String message, String state, String sha) {
  step([
    $class: "GitHubCommitStatusSetter",
    reposSource:   [$class: "ManuallyEnteredRepositorySource", url: "https://github.com/xiantongw/srd"],
    contextSource: [$class: "ManuallyEnteredCommitContextSource", context: "ci/jenkins/build-status"],
    commitShaSource: [$class: "ManuallyEnteredShaSource", sha: sha],
    errorHandlers: [[$class: "ChangingBuildStatusErrorHandler", result: "UNSTABLE"]],
    statusResultSource: [
      $class: "ConditionalStatusResultSource",
      results: [[$class: "AnyBuildResult", message: message, state: state]]
    ]
  ])
}

pipeline {
  agent {
    docker {
      image 'ubuntu:22.04'
      args '-u root:root'
    }
  }
  stages {
    stage('Pre-build') {
      steps {
        script {
          def sha = sh(returnStdout: true, script: 'git rev-parse HEAD || true').trim()
          if (!sha) { sha = env.GIT_COMMIT }
          setBuildStatus("Build started", "PENDING", sha)
        }
      }
    }
    stage('Install dependencies') {
      steps {
        sh '''
          apt-get update
          apt-get install -y curl build-essential git python3
          curl -L https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 -o /usr/local/bin/bazelisk
          chmod +x /usr/local/bin/bazelisk
        '''
      }
    }
    stage('Checkout') {
      steps {
        checkout scm
        script {
          env.BUILD_COMMIT_SHA = sh(returnStdout: true, script: 'git rev-parse HEAD').trim()
          echo "Checked-out SHA: ${env.BUILD_COMMIT_SHA}"
        }
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
      script { setBuildStatus("Build succeeded", "SUCCESS", env.BUILD_COMMIT_SHA ?: env.GIT_COMMIT) }
    }
    failure {
      script { setBuildStatus("Build failed", "FAILURE", env.BUILD_COMMIT_SHA ?: env.GIT_COMMIT) }
    }
  }
}