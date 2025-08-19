#!/bin/bash
# Entrypoint script for zpmod documentation build container
# Handles user/group ownership to match host system

set -e

# Default values
USER_ID=${USER_ID:-1000}
GROUP_ID=${GROUP_ID:-1000}
USER_NAME=${USER_NAME:-docs}
GROUP_NAME=${GROUP_NAME:-docs}

# Function to log messages
log() {
  echo "📝 [entrypoint] $*"
}

# Create group if it doesn't exist
if ! getent group "${GROUP_NAME}" >/dev/null 2>&1; then
  log "Creating group '${GROUP_NAME}' with GID ${GROUP_ID}"
  groupadd -g "${GROUP_ID}" "${GROUP_NAME}"
else
  log "Group '${GROUP_NAME}' already exists"
fi

# Create user if it doesn't exist
if ! getent passwd "${USER_NAME}" >/dev/null 2>&1; then
  log "Creating user '${USER_NAME}' with UID ${USER_ID}"
  useradd -u "${USER_ID}" -g "${GROUP_ID}" -s /bin/bash -m "${USER_NAME}"

  # Add user to sudo group for any necessary privilege escalation
  usermod -aG sudo "${USER_NAME}"
  echo "${USER_NAME} ALL=(ALL) NOPASSWD:ALL" >>/etc/sudoers.d/${USER_NAME}
else
  log "User '${USER_NAME}' already exists"
fi

# Ensure workspace directory permissions
log "Setting workspace permissions for ${USER_NAME}:${GROUP_NAME}"
if [[ -d "/workspace" ]]; then
  # Ensure the user can write to the workspace
  chown -f "${USER_ID}:${GROUP_ID}" /workspace 2>/dev/null || true
fi

# Ensure existing build-cmake directory is owned by the target user (avoids rm permission errors)
if [[ -d "/workspace/build-cmake" ]]; then
  log "Taking ownership of existing build-cmake directory"
  chown -R "${USER_ID}:${GROUP_ID}" /workspace/build-cmake 2>/dev/null || true
fi

# Ensure build-results directory permissions
if [[ -d "/workspace/build-results" ]]; then
  log "Setting build-results permissions"
  chown -R "${USER_ID}:${GROUP_ID}" /workspace/build-results 2>/dev/null || true
fi

# Function to fix ownership of generated files
fix_ownership() {
  log "Fixing ownership of generated files"
  if [[ -d "/workspace/build-cmake" ]]; then
    chown -R "${USER_ID}:${GROUP_ID}" /workspace/build-cmake 2>/dev/null || true
  fi
  if [[ -d "/workspace/build-results" ]]; then
    chown -R "${USER_ID}:${GROUP_ID}" /workspace/build-results 2>/dev/null || true
  fi
}

# Trap to ensure ownership is fixed on exit
trap fix_ownership EXIT

# Switch to the created user and execute the command
log "Switching to user '${USER_NAME}' (UID: ${USER_ID}, GID: ${GROUP_ID})"
log "Executing command: $*"

# Execute the command as the specified user
exec gosu "${USER_NAME}" "$@"
