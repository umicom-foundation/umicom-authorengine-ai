# Backup & Recovery (pragmatic, cross-platform)

- **System image** monthly (Windows: Veeam Agent Free; Linux/macOS: Clonezilla or Time Machine on macOS).
- **File history** daily (Windows File History / rsync/rdiff-backup on Linux / Time Machine on macOS).
- **Offsite sync** with Syncthing or rclone (encrypted remote).
- **Git hygiene**: commit/push often; use Git LFS for large binaries.
