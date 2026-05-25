-- Community renewal: persist status_message for player and player_deleted
-- Adds status_message VARCHAR(50) after last_play to both tables.

START TRANSACTION;

ALTER TABLE `player`
    ADD COLUMN `status_message` VARCHAR(50) NOT NULL DEFAULT '' AFTER `last_play`;

ALTER TABLE `player_deleted`
    ADD COLUMN `status_message` VARCHAR(50) NOT NULL DEFAULT '' AFTER `last_play`;

COMMIT;

-- DOWN (manual rollback):
-- ALTER TABLE `player` DROP COLUMN `status_message`;
-- ALTER TABLE `player_deleted` DROP COLUMN `status_message`;
