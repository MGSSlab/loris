<?php
/**
 * Examiner module menu filter form
 *
 * PHP Version 5
 *
 * @category Clinical
 * @package  Loris
 * @author   Tara Campbell <tara.campbell@mail.mcgill.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/Loris
 */
namespace LORIS\examiner;

/**
 * Examiner module menu filter form
 *
 * PHP Version 5
 *
 * @category Clinical
 * @package  Loris
 * @author   Tara Campbell <tara.campbell@mail.mcgill.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/Loris
 */

class Examiner extends \NDB_Menu_Filter_Form
{
    public $skipTemplate  = true;
    var $useCertification = false;

    /**
     * Checks if the user has access to examiner menu filter
     *
     * @param \User $user The user whose access is being checked
     *
     * @return bool
     */
    function _hasAccess(\User $user) : bool
    {
        return $user->hasAnyPermission(
            [
                'examiner_view',
                'examiner_multisite',
            ]
        );
    }

    /**
     * Sets the menu filter class variables.
     *
     * @return void
     */
    function _setupVariables()
    {
        $config        = \NDB_Config::singleton();
        $certification = $config->getSetting('Certification');

        if (isset($certification['EnableCertification'])) {
            $this->useCertification = $certification['EnableCertification'];
        }

        $user      = \User::singleton();
        $centerIDs = implode(',', $user->getCenterIDs());

        if ($user->hasPermission('examiner_multisite')) {
                $query = "
                        FROM examiners e
                            LEFT JOIN examiners_psc_rel epr ON
                                (epr.examinerID=e.examinerID)
                            LEFT JOIN psc ON (epr.centerID=psc.CenterID)
                            LEFT JOIN certification c ON
                                (c.examinerID=e.examinerID and c.pass = 'certified')
                            LEFT JOIN test_names tn ON (tn.ID = c.testID)
                            LEFT JOIN users u ON u.ID=e.userID
                        WHERE 1=1 ";
        } else {

              $query = "
                        FROM examiners e
                            LEFT JOIN examiners_psc_rel epr ON
                                (epr.examinerID=e.examinerID)
                            LEFT JOIN psc ON (epr.centerID=psc.CenterID)
                            LEFT JOIN certification c ON
                                (c.examinerID=e.examinerID and c.pass = 'certified')
                            LEFT JOIN test_names tn ON (tn.ID = c.testID)
                            LEFT JOIN users u ON u.ID=e.userID
                        WHERE FIND_IN_SET(epr.centerID, '$centerIDs')";
        }

        // set the class variables
        $this->columns      = [
            'e.full_name as Examiner',
            'u.Email',
            'e.examinerID as ID',
            'psc.Name as Site',
            'e.radiologist as Radiologist',
            "GROUP_CONCAT(tn.full_name SEPARATOR ', ') as
                                Certification",
        ];
        $this->group_by     = "e.full_name,psc.Name,e.examinerID";
        $this->query        = $query;
        $this->order_by     = 'e.full_name';
        $this->headers      = [
            'Examiner',
            'Email',
            'ID',
            'Site',
            'Radiologist',
            'Certification',
        ];
        $this->validFilters = [
            'e.full_name',
            'epr.centerID',
            'COALESCE(e.radiologist, 0)',
        ];

        $this->formToFilter = [
            'examiner'    => 'e.full_name',
            'site'        => 'epr.centerID',
            'radiologist' => 'COALESCE(e.radiologist, 0)',
        ];
    }

    /**
     * Does the setup required for this page. By default, sets up elements
     * that are common to every type of page. May be overridden by a specific
     * page or specific page type.
     *
     * @return void
     */
    function setup()
    {
        parent::setup();
        $user = \User::singleton();

        // Get site options
        if ($user->hasPermission('examiner_multisite')) {
            // get the list of study sites - to be replaced by the Site object
            $sites = \Utility::getSiteList();
        } else {
            // allow only to view own site data
            $sites = $user->getStudySites();
        }

        // Set keys of sites to their names for FilterableDataTable
        foreach ($sites as $siteID => $siteName) {
            unset($sites[$siteID]);
            $sites[$siteName] = $siteName;
        }

        // Radiologist options
        $radiologist = [
            '1' => 'Yes',
            '0' => 'No',
        ];

        $this->fieldOptions = [
            'sites'        => $sites,
            'radiologists' => $radiologist,
        ];

        $this->form->addFormRule([&$this, '_validateAddExaminer']);
    }

    /**
     * Validates the examiner form values.
     *
     * @param array $values Add Examiner form values
     *
     * @return void
     */
    function _validateAddExaminer($values)
    {
        $name = $values['addName'];
        $site = $this->getCenterID($values['addSite']);

        if (empty($name) || empty($site)) {
            http_response_code(400);
            echo('Cannot submit an empty name or site.');
            return;
        } else if (trim($name) !== $name || preg_match('/\s[\s]+/', $name) === 1) {
            http_response_code(400);
            echo(
                'Please remove all trailing '.
                'spaces and double spaces in name.'
            );
            return;
        } else if ($this->examinerExists($name, $site)) {
            http_response_code(400);
            echo('This examiner already exists.');
            return;
        }
    }

    /**
     * Processes the add examiner form values
     *
     * @param array $values add examiner form values
     *
     * @return bool true if successful
     */
    function _process($values)
    {
        $factory = \NDB_Factory::singleton();
        $DB      = $factory->database();
        $baseurl = $factory->settings()->getBaseURL();

        $fullName = $values['addName'];
        $siteName = $values['addSite'];

        //Get centerID
        $siteID = $this->getCenterID($siteName);

        //Get radiologist value
        $radiologist = 0;
        if (isset($values['addRadiologist'])
            && $values['addRadiologist'] === 'true'
        ) {
            $radiologist = 1;
        }

        // Check if examiner is already in the database 'examiners', if not add them
        // Assumption made that only non-users are added to the table here
        // Otherwise, would need more information to identify which loris user
        // the name refers to
        $examinerID = $DB->pselectOne(
            "SELECT examinerID
             FROM examiners
             WHERE full_name=:fullName
             AND userID IS NULL",
            ['fullName' => $fullName]
        );

        if (empty($examinerID)) {
            $DB->insert(
                'examiners',
                [
                    'full_name'   => $fullName,
                    'radiologist' => $radiologist
                ]
            );
        } else {
            $DB->update(
                'examiners',
                ['radiologist' => $radiologist],
                ['examinerID' => $examinerID]
            );
        }

        $DB->insert(
            'examiners_psc_rel',
            [
                'examinerID' => $this->getExaminerID($fullName),
                'centerID'   => $siteID,
            ]
        );

        header("Location: {$baseurl}/examiner/", true, 303);
        return true;
    }

    /**
     * Checks if an examiner exists with a given name and site
     *
     * @param string $fullName The full name of the examiner
     * @param int    $siteID   The site ID
     *
     * @return boolean
     */
    function examinerExists($fullName, $siteID)
    {
        $DB         = \NDB_Factory::singleton()->database();
        $examinerID = $DB->pselectOne(
            "SELECT e.examinerID
             FROM examiners e
                  LEFT JOIN examiners_psc_rel epr ON (e.examinerID=epr.examinerID)
             WHERE e.full_name=:fullName
             AND epr.centerID=:siteID
             AND e.userID IS NULL",
            [
                'fullName' => $fullName,
                'siteID'   => $siteID,
            ]
        );

        if (empty($examinerID)) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * Getter for the examinerID value from the database
     *
     * @param string $fullName name associated to examiner in the DB table
     *
     * @return ?string   ID of the examiner
     */
    function getExaminerID($fullName): ?string
    {
        $DB = \NDB_Factory::singleton()->database();
        return $DB->pselectOne(
            "SELECT examinerID
             FROM examiners
             WHERE full_name=:fullName AND userID IS NULL",
            ['fullName' => $fullName]
        );
    }

    /**
     * Getter for the centerID of the center name
     *
     * @param string $siteName name of the site
     *
     * @return ?int
     */
    function getCenterID($siteName): ?int
    {
        $DB = \NDB_Factory::singleton()->database();
        return $DB->pselectOneInt(
            "SELECT CenterID
             FROM psc
             WHERE Name=:siteName",
            ['siteName' => $siteName]
        );
    }

    /**
     * Converts the results of this menu filter to a JSON format to be retrieved
     * with ?format=json
     *
     * @return string JSON
     */
    function toJSON(): string
    {
        $result         = $this->toArray();
        $result['form'] = $this->form->form;

        // Include whether or not certification is being used
        $result['useCertification'] = ($this->useCertification === '1') ?: false;

        return json_encode($result);
    }

    /**
     * Include the index examiner javascript dependency
     *
     * @return array
     */
    function getJSDependencies()
    {
        $factory = \NDB_Factory::singleton();
        $baseURL = $factory->settings()->getBaseURL();
        $deps    = parent::getJSDependencies();
        return array_merge(
            $deps,
            [
                $baseURL . "/examiner/js/examinerIndex.js",
            ]
        );
    }
}

