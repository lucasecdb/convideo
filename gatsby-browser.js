import './src/styles/global.scss'
import { notificationState } from './src/notification/index'

export const onServiceWorkerInstalled = () => {
  notificationState.addNotification({
    message: 'Ready to work offline',
    actionText: 'Dismiss',
  })
}

export const onServiceWorkerUpdateReady = () => {
  notificationState.addNotification({
    message: 'A new update is available!',
    actionText: 'Refresh',
    onAction: () => {
      window.location.reload()
    },
  })
}
