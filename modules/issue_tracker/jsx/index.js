import IssueForm from './IssueForm';

/**
 * Render IssueForm on page load
 */
window.addEventListener('load', () => {
  const id = location.href.split('/issue/')[1];
  ReactDOM.render(
    <IssueForm
      Module='issue_tracker'
      DataURL={loris.BaseURL
        + '/issue_tracker/Edit/?issueID='
              + id}
      action={loris.BaseURL
        + '/issue_tracker/Edit/'}
      issue={id}
      baseURL={loris.BaseURL}
      userHasPermission={loris.userHasPermission('issue_tracker_developer')}
    />,
    document.getElementById('lorisworkspace')
  );
});
