ALTER TABLE visit ADD COLUMN `VisitLabel` VARCHAR(200) UNIQUE;
UPDATE visit SET VisitLabel=VisitName WHERE VisitLabel IS NULL;
ALTER TABLE visit CHANGE `VisitLabel` `VisitLabel` VARCHAR(200) UNIQUE NOT NULL;
