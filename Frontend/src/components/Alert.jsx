export default function Alert({ type = 'error', children, onDismiss }) {
  if (!children) return null
  return (
    <div className={`alert alert-${type}`} role="alert">
      <span>{children}</span>
      {onDismiss && (
        <button type="button" className="alert-dismiss" onClick={onDismiss} aria-label="Dismiss">
          ×
        </button>
      )}
    </div>
  )
}
