import React, { useState, useEffect, useReducer } from 'react'

import Dialog, {
  DialogTitle,
  DialogContent,
  DialogActions,
} from './components/Dialog'
import Button from './components/Button'
import FileUploader from './FileUploader'
import VideoConverter from './VideoConverter'
import NotificationToasts from './NotificationToasts'
import registerSW from './registerSW'
import { notificationState } from './notification/index'

import './App.css'

const FILE_SIZE_LIMIT = 200 * 1024 * 1024 // 200 Mb

type State = {
  file: File | null
  errorTitle?: string
  errorMessage?: string
  errorDialogOpen: boolean
}

const defaultState: State = {
  file: null,
  errorTitle: undefined,
  errorMessage: undefined,
  errorDialogOpen: false,
}

type Action =
  | { type: 'set file'; file: File }
  | { type: 'reset' }
  | { type: 'set error'; title: string; message: string }
  | { type: 'dismiss error' }
  | { type: 'set filesize error' }

const reducer: React.Reducer<State, Action> = (state, action) => {
  switch (action.type) {
    case 'set file': {
      return {
        file: action.file,
        errorTitle: undefined,
        errorMessage: undefined,
        errorDialogOpen: false,
      }
    }
    case 'set error': {
      return {
        ...state,
        errorTitle: action.title,
        errorMessage: action.message,
        errorDialogOpen: true,
      }
    }
    case 'set filesize error': {
      return {
        ...state,
        errorTitle: 'File too large',
        errorMessage:
          'The size of the uploaded file exceeds our limit (200 Mb)',
        errorDialogOpen: true,
      }
    }
    case 'dismiss error': {
      return {
        ...state,
        errorTitle: undefined,
        errorMessage: undefined,
        errorDialogOpen: false,
      }
    }
    case 'reset': {
      return defaultState
    }
    default: {
      return state
    }
  }
}

const App: React.FC = () => {
  const [
    { file, errorTitle, errorMessage, errorDialogOpen },
    dispatch,
  ] = useReducer(reducer, defaultState)

  useEffect(() => {
    let updateNotificationId: string | null = null
    let installNotificationId: string | null = null

    registerSW({
      onUpdate: () => {
        updateNotificationId = notificationState.addNotification({
          message: 'A new update is available!',
          actionText: 'Refresh',
          onAction: () => {
            window.location.reload()
          },
        })
      },
      onInstall: () => {
        installNotificationId = notificationState.addNotification({
          message: 'Ready to work offline',
          actionText: 'Dismiss',
        })
      },
    })

    return () => {
      if (updateNotificationId) {
        notificationState.removeNotification(updateNotificationId)
      }

      if (installNotificationId) {
        notificationState.removeNotification(installNotificationId)
      }
    }
  }, [])

  const handleFile = async (inputFile: File) => {
    if (inputFile.size > FILE_SIZE_LIMIT) {
      dispatch({ type: 'set filesize error' })
      return
    }

    dispatch({ type: 'set file', file: inputFile })
  }

  const handleDialogClose = () => {
    dispatch({ type: 'dismiss error' })
  }

  return (
    <>
      {file ? (
        <VideoConverter
          video={file}
          onClose={() => dispatch({ type: 'reset' })}
        />
      ) : (
        <>
          <FileUploader
            onFile={handleFile}
            onError={message => {
              dispatch({ type: 'set error', title: 'Error', message })
            }}
          />
          <Dialog
            open={errorDialogOpen}
            onClose={handleDialogClose}
            role="alertdialog"
          >
            <DialogTitle>{errorTitle}</DialogTitle>
            <DialogContent>{errorMessage}</DialogContent>
            <DialogActions>
              <Button onClick={handleDialogClose}>Close</Button>
            </DialogActions>
          </Dialog>
        </>
      )}
      <NotificationToasts />
    </>
  )
}

export default App
